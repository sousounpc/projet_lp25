#include  <stdlib.h>
 #include <stdio.h>
 #include <string.h>
 #include <getopt.h>  
#include "manager/manager.h"
#include "process/process.h"



void display_help() {
    printf("Projet LP25\n");
    printf("Syntaxe: ./htop_lp25 [OPTIONS]\n");
}

int main(int argc, char *argv[]) {
    if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        display_help();
        return EXIT_SUCCESS;
    }

    printf("Lancement du gestionnaire de processus en mode local...\n");
    
    // Appel au chef d'orchestre. Le '0' signifie mode normal
    if (manager_run(0) != EXIT_SUCCESS) {
        fprintf(stderr, "Erreur critique lors de l'execution du gestionnaire.\n");
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
