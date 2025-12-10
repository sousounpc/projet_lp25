#include "manager.h"
#include "process/process.h"
#include "ui/ui.h"
#include  <unistd.h>  

static void free_string(char *str)
{
if (str != NULL) {
free(str);
}
}

void manager_free_config(app_config_t *config)
{
remote_target_t *current;
remote_target_t *next;

free_string(config->remote_config_path);
free_string(config->remote_server);
free_string(config->login);
free_string(config->username);
free_string(config->password);

current = config->targets;
while (current != NULL) {
next = current->next;
free_string(current->server_name);
free_string(current->address);
free_string(current->username);
free_string(current->password);
free(current);
current = next;
}
}

int manager_run(app_config_t *config)
{
process_info_t *local_list;

if (config->show_help) {
ui_print_help();
return 0;
}



if (config->dry_run) {
local_list = get_local_process_list();
if (local_list == NULL) {
fprintf(stderr, "Erreur : impossible de lire la liste des processus locaux.\n");
return 1;
}
printf("Dry-run réussi : accès à la liste des processus locaux OK.\n");
free_process_list(local_list);
return 0;
}

local_list = get_local_process_list();
if (local_list == NULL) {
fprintf(stderr, "Erreur : aucun processus trouvé ou /proc inaccessible.\n");
return 1;
}

ui_print_process_table("Machine locale", local_list);

free_process_list(local_list);

return 0;
