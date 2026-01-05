#ifndef PROCESS_H
#define PROCESS_H
#include <sys/types.h>
#include <stddef.h> 
/**
* Constantes de taille pour les chaînes de caractères
*/
#define MAX_CMD_LEN 256
#define MAX_USER_LEN 64

/**
* Structure principale représentant un processus.
* Utilisée à la fois pour le mode Local et le mode Distant.
*/
typedef struct process_info {
pid_t pid;
char cmd[MAX_CMD_LEN];
char user[MAX_USER_LEN];
double cpu_percent;
double mem_percent;
} process_info;

/**
* Collecte les informations de tous les processus locaux via /proc.
* @param out Pointeur vers le tableau de structures qui sera alloué.
* @param count_out Pointeur vers la variable stockant le nombre de processus trouvés.
* @return 0 en cas de succès, -1 en cas d'erreur.
*/
int collect_processes(process_info **out, size_t *count_out);

/**
* @param pid Le PID du processus cible.
* @param signal_type Le type d'action (5=Pause, 6=Reprise, 7=Terminer, 9=Kill).
* @return 0 en cas de succès, -1 en cas d'échec.
*/
int send_signal_to_process(pid_t pid, int signal_type);

#endif
