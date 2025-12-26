#ifndef NETWORK_H
#define NETWORK_H

#include "../process/process.h"

/* * Structure pour gérer les infos de connexion aux machines distantes.
 * Utilisée pour stocker ce qu'on lit dans le fichier de config.
 */
typedef struct remote_host {
    char hostname[256];
    char ip[64];
    int port;
    char username[128];
    char password[128];
    struct remote_host *next; // Pour faire une liste de serveurs
} remote_host_t;

// Charge le fichier .htop_lp25_config (doit être en chmod 600)
remote_host_t *load_network_config(const char *filename);

// Se connecte en SSH sur l'hôte pour récupérer les processus via la commande 'ps'
process_t *fetch_remote_processes(remote_host_t *host);

// Libère la mémoire de la liste des serveurs distants
void free_remote_hosts(remote_host_t *head);

#endif
