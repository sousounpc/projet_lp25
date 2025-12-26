#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <libssh/libssh.h>

// --- 1. LECTURE DU FICHIER DE CONFIGURATION ---

remote_host_t *load_network_config(const char *filename) {
    struct stat st;
    
    // Vérification de l'existence
    if (stat(filename, &st) != 0) {
        return NULL; // Fichier absent, on continue en mode local
    }

    // Sécurité : Vérifier que les permissions sont exactement 600 (rw-------)
    // On vérifie qu'aucun bit n'est mis pour le Groupe ou Others
    if (st.st_mode & (S_IRWXG | S_IRWXO)) {
        fprintf(stderr, "ERREUR SECURITE : Les permissions de %s doivent être 600.\n", filename);
        return NULL;
    }

    FILE *file = fopen(filename, "r");
    if (!file) return NULL;

    remote_host_t *head = NULL, *current = NULL;
    char line[1024];

    // Format attendu par ligne : hostname:ip:port:user:pass
    while (fgets(line, sizeof(line), file)) {
        remote_host_t *new_host = malloc(sizeof(remote_host_t));
        if (sscanf(line, "%255[^:]:%63[^:]:%d:%127[^:]:%127s", 
                   new_host->hostname, new_host->ip, &new_host->port, 
                   new_host->username, new_host->password) == 5) {
            
            new_host->next = NULL;
            if (!head) { head = new_host; current = head; }
            else { current->next = new_host; current = new_host; }
        } else {
            free(new_host);
        }
    }

    fclose(file);
    return head;
}

// --- 2. CONNEXION SSH ET RÉCUPÉRATION DES DONNÉES ---

process_t *fetch_remote_processes(remote_host_t *host) {
    ssh_session session = ssh_new();
    if (session == NULL) return NULL;

    // Configuration de la session
    ssh_options_set(session, SSH_OPTIONS_HOST, host->ip);
    ssh_options_set(session, SSH_OPTIONS_PORT, &host->port);
    ssh_options_set(session, SSH_OPTIONS_USER, host->username);

    // Connexion au serveur
    if (ssh_connect(session) != SSH_OK) {
        ssh_free(session);
        return NULL;
    }

    // Authentification par mot de passe
    if (ssh_userauth_password(session, NULL, host->password) != SSH_AUTH_SUCCESS) {
        ssh_disconnect(session);
        ssh_free(session);
        return NULL;
    }

    // Création d'un canal pour exécuter la commande shell
    ssh_channel channel = ssh_channel_new(session);
    ssh_channel_open_session(channel);
    
    // Commande demandée : PID, USER, %CPU, %MEM, COMM (sans en-tête)
    const char *cmd = "ps -e -o pid,user,%cpu,%mem,comm --no-headers";
    ssh_channel_request_exec(channel, cmd);

    char buffer[4096];
    int nbytes;
    process_t *remote_list = NULL, *last_proc = NULL;

    // Lecture du flux texte renvoyé par le serveur
    while ((nbytes = ssh_channel_read(channel, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[nbytes] = '\0';
        
        // Parsing ligne par ligne
        char *line = strtok(buffer, "\n");
        while (line != NULL) {
            process_t *p = create_process_node();
            // On remplit la structure process_t avec les données parsées
            if (sscanf(line, "%d %255s %f %f %255s", 
                       &p->pid, p->user, &p->cpu_percent, &p->memory_percent, p->command) == 5) {
                
                if (!remote_list) { remote_list = p; last_proc = p; }
                else { last_proc->next = p; last_proc = p; }
            } else {
                free(p);
            }
            line = strtok(NULL, "\n");
        }
    }

    // Fermeture propre
    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    ssh_disconnect(session);
    ssh_free(session);

    return remote_list;
}

void free_remote_hosts(remote_host_t *head) {
    while (head) {
        remote_host_t *tmp = head;
        head = head->next;
        free(tmp);
    }
}
