#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h> 
#include "manager/manager.h"

void display_help() {
    printf("Projet LP25 - Clone htop\n");
    printf("Usage: ./htop_lp25 [OPTIONS]\n");
    printf("  -h, --help      Affiche cette aide\n");
    printf("  -d, --delay MS  Délai de rafraîchissement en ms (ex: 500)\n");
}

int main(int argc, char *argv[]) {
    app_config_t config = {0}; 
    config.delay_ms = 1000; // Valeur par défaut : 1 seconde
    int opt;

    // Parsing des arguments
    while ((opt = getopt(argc, argv, "hd:")) != -1) {
        switch (opt) {
            case 'h':
                config.show_help = true;
                break;
            case 'd':
                config.delay_ms = atoi(optarg);
                if (config.delay_ms < 100) config.delay_ms = 100; // Sécurité
                break;
            default:
                display_help();
                return EXIT_FAILURE;
        }
    }

    if (config.show_help) {
        display_help();
        return EXIT_SUCCESS;
    }

    printf("Lancement du gestionnaire de processus...\n");
    
    // Lancement du Manager
    if (manager_run(&config) != 0) {
        fprintf(stderr, "Erreur critique lors de l'execution.\n");
        manager_free_config(&config);
        return EXIT_FAILURE;
    }
    
    manager_free_config(&config);
    return EXIT_SUCCESS;
}

