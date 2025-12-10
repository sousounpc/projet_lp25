#ifndef MANAGER_H
#define MANAGER_H

#include  <stdbool.h>
typedef enum {
CONNECTION_SSH,
CONNECTION_TELNET
} connection_type_t;

typedef struct remote_target {
char *server_name;
char *address;
int port;
char *username;
char *password;
connection_type_t type;
struct remote_target *next;
} remote_target_t;

typedef struct app_config {
bool show_help;
bool dry_run;
bool use_all;
bool has_remote_server;
bool has_remote_config;
char *remote_config_path;
char *remote_server;
char *login;
char *username;
char *password;
int port;
connection_type_t connection_type;
remote_target_t *targets;
} app_config_t;

int manager_run(app_config_t *config);
void manager_free_config(app_config_t *config);

#endif
