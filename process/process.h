#ifndef PROCESS_H
#define PROCESS_H

 #include <sys/types.h>
 #include <dirent.h>
 #include <ctype.h>


#define MAX_CMD_LEN 256
#define MAX_USER_LEN 64

typedef struct process_info {
pid_t pid;
char cmd[MAX_CMD_LEN];
char user[MAX_USER_LEN];
double cpu_percent;
double mem_percent;
} process_info;

int collect_processes(process_info **out, size_t *count_out);

#endif

