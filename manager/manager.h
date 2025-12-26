#ifndef MANAGER_H
#define MANAGER_H

#include <stdbool.h>
#include <stddef.h>

// Structure pour stocker les infos d'une machine distante (pour l'étudiant C plus tard)
typedef struct remote_target {
    char *server_name;
    char *address;
    char *username;
    char *password;
    int port;
    struct remote_target *next;
} remote_target_t;

// Configuration générale de l'application
typedef struct {
    bool show_help;
    char *remote_config_path;
    
    // Identifiants par défaut (si nécessaire)
    char *remote_server;
    char *login;
    char *username;
    char *password;
    int port;

    // Liste des cibles distantes
    remote_target_t *targets;

    // NOUVEAU : Délai de rafraîchissement (en millisecondes)
    int delay_ms; 

} app_config_t;

// Lance la boucle principale du manager
int manager_run(app_config_t *config);

// Libère la mémoire de la configuration
void manager_free_config(app_config_t *config);

#endif
