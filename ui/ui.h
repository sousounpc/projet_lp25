#ifndef UI_H
#define UI_H

#include "../process/process.h"
#include <sys/types.h>

// Met à jour l'affichage et renvoie la touche pressée
int ui_update(process_info *processes, size_t count);

// Récupère le PID sélectionné
pid_t ui_get_selected_pid(void);

// Récupère l'onglet actif (0=Local, 1..=Distant)
int ui_get_current_tab_index(void);

// Change la vitesse de rafraîchissement (ms)
void ui_set_refresh_rate(int ms);

// Ferme ncurses
void ui_finish(void);

#endif
