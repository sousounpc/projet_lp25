#ifndef NETWORK_H
#define NETWORK_H
#include "../process/process.h"

/* * Structure pour stocker les infos de connexion aux machines distantes.
 * Permet de gérer une liste chaînée de serveurs (champ next).
 */
typedef struct remote_host {
    char hostname[256];
    char ip[64];
    int port;
    char username[128];
    char password[128];
    struct remote_host *next; 
} remote_host_t;
// Add this in network.h if it's missing
typedef struct process_t {
    int pid;
    char user[64];
    char command[256];
    float cpu_percent;
    float memory_percent;
    struct process_t *next;
} process_t;
/* --- Fonctions du module Network --- */

// Lit le fichier de config et vérifie que les permissions sont en 600
remote_host_t *load_network_config(const char *filename);

// Récupère la liste des processus d'un hôte via SSH
process_t *fetch_remote_processes(remote_host_t *host);

// Envoie un signal (Kill/Stop) à un processus sur une machine distante
int send_remote_signal(remote_host_t *host, pid_t pid, int sig);

// Libère la mémoire allouée pour la liste des hôtes
void free_remote_hosts(remote_host_t *head);

#endif // NETWORK_H
