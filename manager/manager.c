#define _POSIX_C_SOURCE 200809L 

#include "manager.h"
#include "../process/process.h"
#include "../network/network.h"
#include "../ui/ui.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ncurses.h>

/**
 * @brief Libère la mémoire d'une chaîne de caractères (sécurisé)
 */
static void free_string(char *str) { 
    if (str) free(str); 
}

/**
 * @brief Gère l'envoi de signaux aux processus (Local uniquement pour l'instant)
 */
void handle_process_action(int sig, char *action_name) {
    // Dans une version complète, il faudrait vérifier si on est en mode distant
    // et envoyer la commande via SSH. Ici, on reste sur du local pour la sécurité.
    pid_t pid = 0; // ui_get_selected_pid() devrait être défini dans ui.h
    if (pid > 0) {
        kill(pid, sig);
    }
}

/**
 * @brief Boucle principale du gestionnaire (M1 + Intégration C)
 */
int manager_run_local(int dry_run_mode) {
    // 1. CHARGEMENT DE LA CONFIGURATION RÉSEAU (Étudiant C)
    // On cherche le fichier sécurisé .htop_lp25_config
    remote_host_t *hosts = load_network_config(".htop_lp25_config");
    
    process_info *processes = NULL;
    int running = 1;

    // Initialisation de l'UI (si pas déjà fait)
    // Note: On passe 'hosts' pour que l'UI puisse créer les onglets dynamiquement
    
    while (running) {
        // 2. IDENTIFICATION DE LA CIBLE (Onglet sélectionné)
        // Onglet 0 = Local, Onglet 1..N = Serveurs distants
        int current_tab = 0; // Simulation: ui_get_current_tab_index();

        if (current_tab == 0) {
            // --- MODE LOCAL ---
            processes = get_all_pids_local();
        } else {
            // --- MODE DISTANT (Étudiant C) ---
            // On trouve l'hôte correspondant à l'onglet
            remote_host_t *target = hosts;
            for (int i = 1; i < current_tab && target != NULL; i++) {
                target = target->next;
            }

            if (target != NULL) {
                // Appel SSH pour récupérer les processus distants
                processes = fetch_remote_processes(target);
            }
        }

        // 3. MISE À JOUR DE L'AFFICHAGE (M3)
        // ui_run_loop prend maintenant la liste (qu'elle soit locale ou distante)
        int key = ui_run_loop(processes);

        // 4. GESTION DES TOUCHES ET ACTIONS
        switch (key) {
            case 'q': 
            case 'Q':
                running = 0;
                break;
            case KEY_F(5): kill(0, SIGSTOP); break; // Simulation action
            case KEY_F(7): kill(0, SIGKILL); break; // Simulation action
        }

        // 5. NETTOYAGE DE L'ITÉRATION
        if (processes) {
            free_process_list(processes);
            processes = NULL;
        }

        // Fréquence de rafraîchissement (1 seconde)
        sleep(1);
    }

    // 6. NETTOYAGE FINAL
    if (hosts) {
        free_remote_hosts(hosts);
    }

    return 0;
}
