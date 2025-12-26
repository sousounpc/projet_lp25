#include "process.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h> 
#include <ctype.h>
#include <sys/types.h>
#include <pwd.h>       // <--- Correction cruciale ici
#include <unistd.h>
#include <dirent.h>

static int is_number_string(const char *s) {
    if (!s || !*s) return 0;
    for (const char *p = s; *p; p++) {
        if (!isdigit((unsigned char)*p)) return 0;
    }
    return 1;
}

static void read_cmd(pid_t pid, char *buf, size_t len) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/comm", pid);
    FILE *f = fopen(path, "r");
    if (!f) { strncpy(buf, "?", len); return; }
    if (fgets(buf, len, f)) buf[strcspn(buf, "\n")] = 0;
    fclose(f);
}

static void read_user(pid_t pid, char *buf, size_t len) {
    char path[256], line[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *f = fopen(path, "r");
    if (!f) { strncpy(buf, "?", len); return; }
    
    uid_t uid = (uid_t)-1;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Uid:", 4) == 0) {
            sscanf(line + 4, "%d", &uid);
            break;
        }
    }
    fclose(f);

    if (uid != (uid_t)-1) {
        struct passwd *pw = getpwuid(uid);
        if (pw) strncpy(buf, pw->pw_name, len);
        else snprintf(buf, len, "%d", uid);
    } else {
        strncpy(buf, "?", len);
    }
}

int collect_processes(process_info **out, size_t *count_out) {
    DIR *d = opendir("/proc");
    if (!d) return -1;

    process_info *arr = malloc(sizeof(process_info) * 2048);
    if (!arr) { closedir(d); return -1; }

    size_t count = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (!is_number_string(ent->d_name)) continue;
        pid_t pid = atoi(ent->d_name);
        if (pid <= 0) continue;

        process_info p;
        p.pid = pid;
        memset(p.cmd, 0, MAX_CMD_LEN);
        memset(p.user, 0, MAX_USER_LEN);

        read_cmd(pid, p.cmd, MAX_CMD_LEN);
        read_user(pid, p.user, MAX_USER_LEN);
        p.cpu_percent = 0.0;
        p.mem_percent = 0.0;
        
        arr[count++] = p;
        if (count >= 2048) break;
    }

    closedir(d);
    *out = arr;
    *count_out = count;
    return 0;
}
