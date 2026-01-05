#define _POSIX_C_SOURCE 200809L 

#include "manager.h"
#include "../process/process.h"
#include "../ui/ui.h"
#include "../network/network.h" // Intégration de l'étudiant C
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <ncurses.h>




void manager_free_config(app_config_t *config) {
    // Nettoyage de la config réseau chargée
    // Note: remote_targets dans config pointe vers la structure de network.h
    // Si network.h fournit une fonction de nettoyage, on l'utilise.
    if (config->targets) {
        free_remote_hosts((remote_host_t *)config->targets);
    }
}

void handle_process_action(int signal, char *action_name) {
    pid_t pid = ui_get_selected_pid();
    if (pid > 0) {
        // En local on utilise kill, en distant on devrait utiliser send_remote_signal
        // Pour l'instant, on suppose que l'action est locale ou gérée
        kill(pid, signal);
    }
}

/* * Helper: Convertit la liste chaînée de l'étudiant C (process_t*) 
 * en tableau pour l'UI de l'étudiant B (process_info*) 
 */
int convert_network_list_to_array(process_t *head, process_info **out_arr) {
    if (!head) return 0;

    // 1. Compter les éléments
    int count = 0;
    process_t *curr = head;
    while (curr) { count++; curr = curr->next; }

    // 2. Allouer le tableau
    process_info *arr = malloc(sizeof(process_info) * count);
    if (!arr) return 0;

    // 3. Remplir le tableau
    curr = head;
    int i = 0;
    while (curr && i < count) {
        arr[i].pid = curr->pid;
        strncpy(arr[i].user, curr->user, MAX_USER_LEN - 1);
        strncpy(arr[i].cmd, curr->command, MAX_CMD_LEN - 1);
        arr[i].cpu_percent = curr->cpu_percent;
        arr[i].mem_percent = curr->memory_percent;
        
        curr = curr->next;
        i++;
    }

    *out_arr = arr;
    return count;
}

// Helper pour libérer la liste de l'étudiant C
void free_network_list(process_t *head) {
    while (head) {
        process_t *tmp = head;
        head = head->next;
        free(tmp);
    }
}

int manager_run(app_config_t *config) {
    process_info *processes = NULL;
    size_t count = 0;
    int running = 1;

    if (config->show_help) return 0;

    // 1. Chargement de la config réseau (Travail Étudiant C)
    // On stocke le résultat dans config->targets
    config->targets = (remote_target_t *)load_network_config(".htop_lp25_config");

    // Appliquer la vitesse choisie dans le main
    ui_set_refresh_rate(config->delay_ms);

    while (running) {
        int current_tab = ui_get_current_tab_index();

        // Nettoyage précédent
        if (processes) { free(processes); processes = NULL; count = 0; }

        if (current_tab == 0) {
            // --- ONGLET LOCAL (Étudiant A) ---
            if (collect_processes(&processes, &count) != 0) {
                ui_finish();
                fprintf(stderr, "Erreur critique: lecture /proc échouée.\n");
                return 1;
            }
        } 
        else {
            // --- ONGLETS DISTANTS (Étudiant C) ---
            remote_host_t *target = (remote_host_t *)config->targets;
            
            // On avance dans la liste chaînée pour trouver le bon serveur
            // Tab 1 = Serveur 1, Tab 2 = Serveur 2...
            for (int i = 1; i < current_tab && target != NULL; i++) {
                target = target->next;
            }

            if (target) {
                // Récupération via SSH
                process_t *remote_list = fetch_remote_processes(target);
                
                // Conversion Liste -> Tableau pour l'UI
                if (remote_list) {
                    count = convert_network_list_to_array(remote_list, &processes);
                    // On libère la liste brute de l'étudiant C car on a notre tableau
                    free_network_list(remote_list); 
                }
            }
        }

        // 2. Mise à jour UI
        int key = ui_update(processes, count);

        // 3. Gestion des touches
        switch (key) {
            case 'q': 
            case 'Q':
                running = 0;
                break;
            case KEY_F(5): handle_process_action(SIGSTOP, "PAUSE"); break;
            case KEY_F(6): handle_process_action(SIGCONT, "CONTINUE"); break;
            case KEY_F(7): handle_process_action(SIGKILL, "KILL"); break;
            case KEY_F(8): handle_process_action(SIGHUP, "RESTART"); break;
        }
    }

    ui_finish();
    return 0;
}
