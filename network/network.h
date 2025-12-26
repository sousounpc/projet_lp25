#ifndef NETWORK_H
#define NETWORK_H

#include "../process/process.h"

// Structure pour stocker les informations de connexion d'un hôte distant
typedef struct remote_host {
    char hostname[256];
    char ip[64];
    int port;
    char username[128];
    char password[128];
    struct remote_host *next;
} remote_host_t;

/**
 * Lit le fichier de configuration et vérifie les permissions (600).
 * @param filename Chemin du fichier de configuration.
 * @return Liste chaînée des hôtes configurés.
 */
remote_host_t *load_network_config(const char *filename);

/**
 * Se connecte à un hôte distant via SSH et récupère la liste des processus.
 * @param host Informations sur l'hôte distant.
 * @return Liste chaînée de processus distants.
 */
process_t *fetch_remote_processes(remote_host_t *host);

/**
 * Libère la mémoire de la liste des hôtes.
 */
void free_remote_hosts(remote_host_t *head);

#endif // NETWORK_H
