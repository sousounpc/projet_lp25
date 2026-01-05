#include "process.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <dirent.h>


typedef struct {
    pid_t pid;
    unsigned long long utime;
    unsigned long long stime;
} pid_history_t;

static unsigned long long prev_total_tick = 0;
static pid_history_t history[2048];
static int history_count = 0;

/**
 * FONCTIONS AUXILIAIRES DE LECTURE SYSTÈME
 */

static int is_number_string(const char *s) {
    if (!s || !*s) return 0;
    for (const char *p = s; *p; p++) if (!isdigit((unsigned char)*p)) return 0;
    return 1;
}

static unsigned long long get_sys_total_ticks() {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return 0;
    char cpu[10];
    unsigned long long u, n, s, i, io, irq, sirq;
    if (fscanf(f, "%s %llu %llu %llu %llu %llu %llu %llu", cpu, &u, &n, &s, &i, &io, &irq, &sirq) < 8) {
        fclose(f); return 0;
    }
    fclose(f);
    return u + n + s + i + io + irq + sirq;
}

// Lit les ticks utime/stime d'un processus spécifique
static void get_pid_ticks(pid_t pid, unsigned long long *u, unsigned long long *s) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *f = fopen(path, "r");
    if (!f) { *u = 0; *s = 0; return; }
   
    char buf[1024];
    if (!fgets(buf, sizeof(buf), f)) { fclose(f); return; }
    fclose(f);

    char *last_paren = strrchr(buf, ')');
    if (!last_paren) return;
    // utime (14) et stime (15) se trouvent après le nom entre parenthèses
    sscanf(last_paren + 2, "%*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %llu %llu", u, s);
}

/**
 * COLLECTE DES PROCESSUS ET CALCUL DES MÉTRIQUES
 */
int collect_processes(process_info **out, size_t *count_out) {
    DIR *d = opendir("/proc");
    if (!d) return -1;

    unsigned long long current_total_tick = get_sys_total_ticks();
    unsigned long long delta_total = current_total_tick - prev_total_tick;
   
    process_info *arr = malloc(sizeof(process_info) * 2048);
    if (!arr) { closedir(d); return -1; }

    size_t count = 0;
    struct dirent *ent;
    pid_history_t next_history[2048];
    int next_history_count = 0;

    while ((ent = readdir(d)) != NULL && count < 2048) {
        if (!is_number_string(ent->d_name)) continue;
        pid_t pid = atoi(ent->d_name);

        process_info p;
        p.pid = pid;
       
        // --- 1. NOM ET UTILISATEUR ---
        char path[256];
        snprintf(path, sizeof(path), "/proc/%d/comm", pid);
        FILE *fc = fopen(path, "r");
        if (fc) { if (fgets(p.cmd, MAX_CMD_LEN, fc)) p.cmd[strcspn(p.cmd, "\n")] = 0; fclose(fc); }

        snprintf(path, sizeof(path), "/proc/%d/status", pid);
        FILE *fs = fopen(path, "r");
        if (fs) {
            char line[256]; uid_t uid = -1;
            while(fgets(line, sizeof(line), fs)) {
                if (strncmp(line, "Uid:", 4) == 0) { sscanf(line+4, "%u", &uid); break; }
            }
            fclose(fs);
            struct passwd *pw = getpwuid(uid);
            if (pw) strncpy(p.user, pw->pw_name, MAX_USER_LEN); else snprintf(p.user, MAX_USER_LEN, "%u", uid);
        }

        // --- 2. CALCUL CPU% (Algorithme du Delta) ---
        unsigned long long u_now, s_now;
        get_pid_ticks(pid, &u_now, &s_now);
        unsigned long long proc_total_now = u_now + s_now;

        p.cpu_percent = 0.0;
        for (int i = 0; i < history_count; i++) {
            if (history[i].pid == pid) {
                unsigned long long delta_proc = proc_total_now - (history[i].utime + history[i].stime);
                if (delta_total > 0)
                    p.cpu_percent = ((double)delta_proc / (double)delta_total) * 100.0;
                break;
            }
        }
       
        if (next_history_count < 2048) {
            next_history[next_history_count].pid = pid;
            next_history[next_history_count].utime = u_now;
            next_history[next_history_count].stime = s_now;
            next_history_count++;
        }

        // --- 3. CALCUL MEM% (Usage RSS en Mo) ---
        snprintf(path, sizeof(path), "/proc/%d/statm", pid);
        FILE *fm = fopen(path, "r");
        if (fm) {
            long rss;
            if (fscanf(fm, "%*d %ld", &rss) == 1) {
                p.mem_percent = (double)(rss * sysconf(_SC_PAGESIZE)) / (1024.0 * 1024.0);
            }
            fclose(fm);
        }

        arr[count++] = p;
    }

    // Mise à jour de l'état statique pour le prochain appel
    prev_total_tick = current_total_tick;
    history_count = next_history_count;
    memcpy(history, next_history, sizeof(pid_history_t) * next_history_count);

    closedir(d);
    *out = arr;
    *count_out = count;
    return 0;
}

/**
 * GESTION DES ACTIONS (Signaux)
 */
int send_signal_to_process(pid_t pid, int signal_type) {
    if (pid <= 0) return -1;
   
    int sig;
    switch (signal_type) {
        case 5: sig = SIGSTOP; break; // Pause (F5)
        case 6: sig = SIGCONT; break; // Reprise (F6)
        case 7: sig = SIGTERM; break; // Fin propre (F7)
        case 9: sig = SIGKILL; break; // Force (F8)
        default: return -1;
    }
    return kill(pid, sig);
}
int send_signal_to_process(pid_t pid, int signal_type);
