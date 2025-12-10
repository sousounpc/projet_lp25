#include "ui.h"
#include "../manager/manager.h"
#include <ncurses.h>

#define COLOR_TITLE 1
#define COLOR_ROW_HIGHLIGHT 2


static void init_ui() {
initscr();
cbreak();
noecho();
keypad(stdscr, TRUE);
curs_set(0);

if (has_colors()) {
start_color();
init_pair(COLOR_TITLE, COLOR_CYAN, COLOR_BLACK);
init_pair(COLOR_ROW_HIGHLIGHT, COLOR_BLACK, COLOR_WHITE);
}
}


static void draw_process_table(process_t *head) {
int row = 3;
process_t *current = head;


attron(A_BOLD | COLOR_PAIR(COLOR_TITLE));
mvprintw(1, 2, "PID");
mvprintw(1, 10, "USER");
mvprintw(1, 25, "CPU%%");
mvprintw(1, 35, "MEM%%");
mvprintw(1, 45, "COMMANDE");
attroff(A_BOLD | COLOR_PAIR(COLOR_TITLE));


while (current != NULL) {
mvprintw(row, 2, "%d", current->pid);
mvprintw(row, 10, "%s", current->user);
mvprintw(row, 25, "%.1f", current->cpu_percent);
mvprintw(row, 35, "%.1f", current->memory_percent);
mvprintw(row, 45, "%.*s", COLS - 47, current->command);

row++;
current = current->next;
}
}


static void handle_input(int key, pid_t selected_pid) {
if (selected_pid == 0) return;

// F7 : Kill
if (key == KEY_F(7)) {
if (control_kill_process(selected_pid) == 0) {
mvprintw(LINES - 2, 0, "SUCCESS: Processus %d termine (F7).", selected_pid);
} else {
mvprintw(LINES - 2, 0, "ERREUR: Echec de la termination du processus %d.", selected_pid);
}
}

// F5 : Pause
if (key == KEY_F(5)) {
if (control_pause_process(selected_pid) == 0) {
mvprintw(LINES - 2, 0, "SUCCESS: Processus %d mis en pause (F5).", selected_pid);
} else {
mvprintw(LINES - 2, 0, "ERREUR: Echec de la pause du processus %d.", selected_pid);
}
}

// Q : Quitter
if (key == 'q' || key == 'Q') {
// Le manager gère la sortie
}
}


int ui_run_loop(process_t *head) {
static int initialized = 0;

if (!initialized) {
init_ui();
initialized = 1;
}

// Effacer l'écran
clear();

// Afficher le titre et les informations
attron(COLOR_PAIR(COLOR_TITLE) | A_BOLD);
mvprintw(0, 0, " HTOP LP25 - Rendu Local | Appuyez sur Q pour Quitter | F7 pour Tuer le PID 100 ");
attroff(COLOR_PAIR(COLOR_TITLE) | A_BOLD);

// Dessiner le tableau des processus
draw_process_table(head);

// Afficher le message d'aide
mvprintw(LINES - 1, 0, "Appuyez sur Q pour quitter. F7 pour tuer le PID (Simule: 100).");

// Gérer l'entrée utilisateur
timeout(100); // Attendre 100ms pour une touche, puis continuer la boucle
int ch = getch();


handle_input(ch, 100);

refresh(); // Mettre à jour l'écran

// Si l'utilisateur a tapé 'q', retourner un signal d'arrêt au manager
if (ch == 'q' || ch == 'Q') {
endwin(); // Nettoyer ncurses
return 1; // Signal de sortie
}

return 0; // Continuer la boucle
}

