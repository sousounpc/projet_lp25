

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

// Structure pour sauvegarder l'état précédent du CPU (pour le calcul du %)
typedef struct {
    pid_t pid;
    unsigned long long utime;
    unsigned long long stime;
} pid_history_t;

static unsigned long long prev_sys_total_ticks = 0;
static pid_history_t history[4096]; // Augmenté pour gérer plus de processus
static int history_count = 0;

/**
 * --- FONCTIONS AUXILIAIRES ---
 */

static int is_number_string(const char *s) {
    if (!s || !*s) return 0;
    for (const char *p = s; *p; p++) if (!isdigit((unsigned char)*p)) return 0;
    return 1;
}

// Récupère le temps total du processeur (user + nice + system + idle...)
static unsigned long long get_sys_total_ticks() {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return 0;
    char label[10];
    unsigned long long u, n, s, i, io, irq, sirq, steal;
    // On lit la première ligne "cpu  ..."
    if (fscanf(f, "%s %llu %llu %llu %llu %llu %llu %llu %llu", 
               label, &u, &n, &s, &i, &io, &irq, &sirq, &steal) < 9) {
        fclose(f); return 0;
    }
    fclose(f);
    return u + n + s + i + io + irq + sirq + steal;
}

// Récupère les ticks CPU spécifiques à un PID
static void get_pid_ticks(pid_t pid, unsigned long long *u, unsigned long long *s) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *f = fopen(path, "r");
    if (!f) { *u = 0; *s = 0; return; }
   
    char buf[1024];
    if (!fgets(buf, sizeof(buf), f)) { fclose(f); return; }
    fclose(f);

    // Le nom du processus est entre parenthèses (...)
    // On cherche la dernière parenthèse fermante pour éviter les bugs si le nom contient ')'
    char *last_paren = strrchr(buf, ')');
    if (!last_paren) return;
    
    // utime est le 14ème champ, stime le 15ème
    // Après la parenthèse, on saute : state, ppid, pgrp, session, tty_nr, tpgid, flags, minflt, cminflt, majflt, cmajflt
    // Soit 11 champs à sauter.
    sscanf(last_paren + 2, "%*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %llu %llu", u, s);
}

// Récupère la RAM totale du système en Pages
static long get_total_system_ram_pages() {
    return sysconf(_SC_PHYS_PAGES);
}

/**
 * --- FONCTION PRINCIPALE ---
 */
int collect_processes(process_info **out, size_t *count_out) {
    DIR *d = opendir("/proc");
    if (!d) return -1;

    // 1. Gestion du temps global (Delta Total)
    unsigned long long current_sys_total = get_sys_total_ticks();
    unsigned long long delta_sys = current_sys_total - prev_sys_total_ticks;
    
    // Sécurité première exécution
    if (prev_sys_total_ticks == 0) delta_sys = 0; 

    // Info mémoire statique
    long total_ram_pages = get_total_system_ram_pages();

    process_info *arr = malloc(sizeof(process_info) * 1024); // Max 1024 processus
    if (!arr) { closedir(d); return -1; }

    size_t count = 0;
    struct dirent *ent;
    
    // Préparation nouvel historique
    pid_history_t next_history[4096];
    int next_history_count = 0;

    while ((ent = readdir(d)) != NULL && count < 1024) {
        if (!is_number_string(ent->d_name)) continue;
        pid_t pid = atoi(ent->d_name);

        process_info p;
        p.pid = pid;
        p.cpu_percent = 0.0;
        p.mem_percent = 0.0;
       
        // --- LECTURE COMMANDE ---
        char path[256];
        snprintf(path, sizeof(path), "/proc/%d/comm", pid);
        FILE *fc = fopen(path, "r");
        if (fc) { 
            if (fgets(p.cmd, MAX_CMD_LEN, fc)) p.cmd[strcspn(p.cmd, "\n")] = 0; 
            fclose(fc); 
        } else {
            strcpy(p.cmd, "?");
        }

        // --- LECTURE UTILISATEUR ---
        snprintf(path, sizeof(path), "/proc/%d/status", pid);
        FILE *fs = fopen(path, "r");
        if (fs) {
            char line[256]; uid_t uid = -1;
            while(fgets(line, sizeof(line), fs)) {
                if (strncmp(line, "Uid:", 4) == 0) { sscanf(line+4, "%u", &uid); break; }
            }
            fclose(fs);
            struct passwd *pw = getpwuid(uid);
            if (pw) strncpy(p.user, pw->pw_name, MAX_USER_LEN); 
            else snprintf(p.user, MAX_USER_LEN, "%u", uid);
        } else {
            strcpy(p.user, "unknown");
        }

        // --- CALCUL CPU % ---
        unsigned long long u_now, s_now;
        get_pid_ticks(pid, &u_now, &s_now);
        unsigned long long proc_total_now = u_now + s_now;

        // On cherche ce PID dans l'historique précédent
        if (delta_sys > 0) {
            for (int i = 0; i < history_count; i++) {
                if (history[i].pid == pid) {
                    unsigned long long prev_proc_total = history[i].utime + history[i].stime;
                    // Delta Processus
                    if (proc_total_now >= prev_proc_total) {
                        unsigned long long delta_proc = proc_total_now - prev_proc_total;
                        // Formule : (Delta Proc / Delta System) * Nb_Coeurs * 100 
                        // Note: top divise par le temps total. Ici simplifions.
                        p.cpu_percent = ((double)delta_proc / (double)delta_sys) * 100.0;
                        // Ajustement multi-coeur souvent nécessaire, mais commençons simple.
                        // Si > 100% (multi-thread), c'est possible sous Linux.
                    }
                    break;
                }
            }
        }

        // Sauvegarde pour la prochaine fois
        if (next_history_count < 4096) {
            next_history[next_history_count].pid = pid;
            next_history[next_history_count].utime = u_now;
            next_history[next_history_count].stime = s_now;
            next_history_count++;
        }

        // --- CALCUL MEM % ---
        snprintf(path, sizeof(path), "/proc/%d/statm", pid);
        FILE *fm = fopen(path, "r");
        if (fm) {
            long rss_pages;
            // Le 2ème champ de statm est la RSS en pages
            if (fscanf(fm, "%*d %ld", &rss_pages) == 1) {
                if (total_ram_pages > 0) {
                    p.mem_percent = ((double)rss_pages / (double)total_ram_pages) * 100.0;
                }
            }
            fclose(fm);
        }

        arr[count++] = p;
    }

    // Mise à jour de l'état statique
    prev_sys_total_ticks = current_sys_total;
    history_count = next_history_count;
    memcpy(history, next_history, sizeof(pid_history_t) * next_history_count);

    closedir(d);
    *out = arr;
    *count_out = count;
    return 0;
}

int send_signal_to_process(pid_t pid, int signal_type) {
    if (pid <= 0) return -1;
    return kill(pid, signal_type); // Simplification directe
}


