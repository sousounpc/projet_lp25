// Indispensable pour kill
#define _POSIX_C_SOURCE 200809L 

#include "ui.h"
#include "../process/process.h"
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_DISPLAY_PROCS 1024
#define COLOR_TITLE 1
#define COLOR_HEADER 2
#define COLOR_SELECTED 3
#define COLOR_DEFAULT 4
#define COLOR_RED_BG 5
#define COLOR_GREEN_TXT 6

/* 1. DÉFINITION DE LA STRUCTURE ET VARIABLES GLOBALES */
typedef struct {
    int current_tab;
    int selected_line;
    int scroll_offset;
    int filter_active;
    char filter_text[32];
    int sort_mode; // 0=PID, 1=CPU, 2=MEM
    int max_rows;
} ui_context_t;

static ui_context_t ui_state; 
static process_info display_procs[MAX_DISPLAY_PROCS];
static int current_proc_count = 0;
static int ui_refresh_rate = 1000;

/* Fonctions de tri */
int compare_pid(const void *a, const void *b) {
    return (((const process_info *)a)->pid - ((const process_info *)b)->pid);
}
int compare_cpu(const void *a, const void *b) {
    double diff = ((const process_info *)b)->cpu_percent - ((const process_info *)a)->cpu_percent;
    if (diff > 0) return 1;
    if (diff < 0) return -1;
    return 0;
}
int compare_mem(const void *a, const void *b) {
    double diff = ((const process_info *)b)->mem_percent - ((const process_info *)a)->mem_percent;
    if (diff > 0) return 1;
    if (diff < 0) return -1;
    return 0;
}

/* Logique interne */
static int get_filtered_count(void) {
    if (!ui_state.filter_active) return current_proc_count;
    int count = 0;
    for (int i = 0; i < current_proc_count; i++) {
        if (strstr(display_procs[i].cmd, ui_state.filter_text) || 
            strstr(display_procs[i].user, ui_state.filter_text)) count++;
    }
    return count;
}

static int get_selected_pid(void) {
    int matches_found = 0;
    for (int i = 0; i < current_proc_count; ++i) {
        if (ui_state.filter_active) {
            if (!strstr(display_procs[i].cmd, ui_state.filter_text) && 
                !strstr(display_procs[i].user, ui_state.filter_text)) continue;
        }
        if (matches_found < ui_state.scroll_offset) { matches_found++; continue; }
        
        if ((matches_found - ui_state.scroll_offset) == (ui_state.selected_line - ui_state.scroll_offset)) {
            return display_procs[i].pid;
        }
        matches_found++;
    }
    return -1;
}

/* Affichage */
static void init_ui_internal() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    start_color();
    timeout(ui_refresh_rate); 

    init_pair(COLOR_TITLE, COLOR_BLACK, COLOR_CYAN);   
    init_pair(COLOR_HEADER, COLOR_BLUE, COLOR_BLACK);
    init_pair(COLOR_SELECTED, COLOR_BLACK, COLOR_WHITE); 
    init_pair(COLOR_DEFAULT, COLOR_WHITE, COLOR_BLACK); 
    init_pair(COLOR_RED_BG, COLOR_WHITE, COLOR_RED);    
    init_pair(COLOR_GREEN_TXT, COLOR_GREEN, COLOR_BLACK); 

    ui_state.current_tab = 0;
    ui_state.selected_line = 0;
    ui_state.scroll_offset = 0;
    ui_state.sort_mode = 0; 
    memset(ui_state.filter_text, 0, sizeof(ui_state.filter_text));
}

static void show_popup(const char *message) {
    int y = (LINES - 6) / 2;
    int x = (COLS - 44) / 2;
    attron(COLOR_PAIR(COLOR_RED_BG));
    mvprintw(y + 2, x + 2, "%-40s", message);
    attroff(COLOR_PAIR(COLOR_RED_BG));
    refresh();
    timeout(-1); getch(); timeout(ui_refresh_rate);
}

static void prompt_filter(void) {
    timeout(-1); echo(); curs_set(1); 
    mvprintw(LINES-1, 0, "Filtre: ");
    getnstr(ui_state.filter_text, 30); 
    ui_state.filter_active = (strlen(ui_state.filter_text) > 0);
    noecho(); curs_set(0); timeout(ui_refresh_rate);
    ui_state.selected_line = 0; ui_state.scroll_offset = 0;
}

static void draw_tabs(void) {
    mvprintw(0, 0, " LP25 Manager %s", ui_state.filter_active ? "[FILTRE ACTIF]" : "");
    const char *tabs[] = {"LOCAL", "SRV_DIST_1", "SRV_DIST_2"};
    move(2, 2);
    for (int i = 0; i < 3; ++i) {
        if (i == ui_state.current_tab) attron(COLOR_PAIR(COLOR_HEADER));
        printw(" [%s] ", tabs[i]);
        if (i == ui_state.current_tab) attroff(COLOR_PAIR(COLOR_HEADER));
        printw(" ");
    }
}

static void draw_process_list(void) {
    int max_rows = LINES - 7;
    ui_state.max_rows = max_rows;

    // 1. APPLIQUER LE TRI
    if (ui_state.sort_mode == 1) qsort(display_procs, current_proc_count, sizeof(process_info), compare_cpu);
    else if (ui_state.sort_mode == 2) qsort(display_procs, current_proc_count, sizeof(process_info), compare_mem);
    else qsort(display_procs, current_proc_count, sizeof(process_info), compare_pid);

    // 2. EN-TÊTES (AVEC SURBRILLANCE)
    move(4, 1);
    
    if (ui_state.sort_mode == 0) attron(A_REVERSE); 
    printw("%-6s", "PID");
    if (ui_state.sort_mode == 0) attroff(A_REVERSE);

    printw(" %-10s ", "USER");

    if (ui_state.sort_mode == 1) attron(A_REVERSE); 
    printw("%-10s", "CPU%");
    if (ui_state.sort_mode == 1) attroff(A_REVERSE);

    printw(" ");

    if (ui_state.sort_mode == 2) attron(A_REVERSE); 
    printw("%-10s", "MEM%");
    if (ui_state.sort_mode == 2) attroff(A_REVERSE);

    printw(" %-20s", "CMD");

    // 3. LISTE
    int matches = 0, drawn = 0;
    for (int i = 0; i < current_proc_count; ++i) {
        if (ui_state.filter_active && !strstr(display_procs[i].cmd, ui_state.filter_text)) continue;
        if (matches++ < ui_state.scroll_offset) continue;
        if (drawn >= max_rows) break;

        if (drawn == (ui_state.selected_line - ui_state.scroll_offset)) attron(COLOR_PAIR(COLOR_SELECTED));
        
        mvprintw(5 + drawn, 1, "%-6d %-10s %5.1f      %5.1f      %-.*s", 
            display_procs[i].pid, display_procs[i].user, 
            display_procs[i].cpu_percent, display_procs[i].mem_percent, 
            COLS-50, display_procs[i].cmd);
        
        attroff(COLOR_PAIR(COLOR_SELECTED));
        drawn++;
    }
    
    // --- ZONE MODIFIÉE POUR AFFICHER TOUTES LES TOUCHES ---
    mvprintw(LINES - 1, 1, "F1:Help F2:Tab F4:Filt F5:Paus F6:Cont F7:Kill P:Cpu M:Mem Q:Quit");
}

/* API PUBLIQUE */
void ui_update_data(process_info *new_data, size_t count) {
    if (count > MAX_DISPLAY_PROCS) count = MAX_DISPLAY_PROCS;
    current_proc_count = (int)count;
    for (int i=0; i<current_proc_count; i++) display_procs[i] = new_data[i];
}

void ui_set_refresh_rate(int ms) {
    if (ms < 100) ms = 100;
    ui_refresh_rate = ms;
    timeout(ui_refresh_rate);
}

int ui_update(process_info *processes, size_t count) {
    static int initialized = 0;
    if (!initialized) { init_ui_internal(); initialized = 1; }

    if (processes) ui_update_data(processes, count);

    erase();
    draw_tabs();
    draw_process_list();
    refresh();

    int ch = getch();
    if (ch == ERR) return 0;

    int total = get_filtered_count();

    switch (ch) {
        case KEY_UP:
            if (ui_state.selected_line > 0) {
                ui_state.selected_line--;
                if (ui_state.selected_line < ui_state.scroll_offset) ui_state.scroll_offset--;
            }
            break;
        case KEY_DOWN:
            if (total > 0 && ui_state.selected_line < total - 1) {
                ui_state.selected_line++;
                if (ui_state.selected_line >= ui_state.scroll_offset + ui_state.max_rows) ui_state.scroll_offset++;
            }
            break;
        case KEY_F(2): ui_state.current_tab = (ui_state.current_tab + 1) % 3; break;
        case KEY_F(3): ui_state.current_tab = (ui_state.current_tab + 2) % 3; break; 
        case KEY_F(4): prompt_filter(); break;
        
        case 'p': case 'P': ui_state.sort_mode = 1; break;
        case 'm': case 'M': ui_state.sort_mode = 2; break;
        case 'n': case 'N': ui_state.sort_mode = 0; break;
        
        case KEY_F(1): show_popup("Touches: F2/Tab=Onglet F4=Filtre P/M=Tri F5/F7=Action"); break;
    }

    return ch; 
}

pid_t ui_get_selected_pid(void) { return get_selected_pid(); }
int ui_get_current_tab_index(void) { return ui_state.current_tab; }
void ui_finish(void) { endwin(); }
