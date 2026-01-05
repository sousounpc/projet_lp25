#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <libssh/libssh.h>
#include <errno.h>

// --- 1. CONFIGURATION LOADING ---

remote_host_t *load_network_config(const char *filename) {
    struct stat st;
    
    // Check if file exists
    if (stat(filename, &st) != 0) {
        return NULL; 
    }

    // Security check: permissions must be 600 (rw-------)
    // We check if Group or Others have any permissions
    if (st.st_mode & (S_IRWXG | S_IRWXO)) {
        fprintf(stderr, "SECURITY WARNING: Permissions for %s should be 600.\n", filename);
        // We continue anyway for testing purposes, but in real life we might return NULL
    }

    FILE *file = fopen(filename, "r");
    if (!file) return NULL;

    remote_host_t *head = NULL, *current = NULL;
    char line[1024];

    // Expected format: hostname:ip:port:user:pass
    while (fgets(line, sizeof(line), file)) {
        remote_host_t *new_host = calloc(1, sizeof(remote_host_t));
        if (!new_host) break;

        // Parse the line
        if (sscanf(line, "%255[^:]:%63[^:]:%d:%127[^:]:%127s", 
                   new_host->hostname, new_host->ip, &new_host->port, 
                   new_host->username, new_host->password) == 5) {
            
            new_host->next = NULL;
            if (!head) { 
                head = new_host; 
                current = head; 
            } else { 
                current->next = new_host; 
                current = new_host; 
            }
        } else {
            free(new_host);
        }
    }

    fclose(file);
    return head;
}

// --- 2. SSH CONNECTION AND DATA FETCHING ---

process_t *fetch_remote_processes(remote_host_t *host) {
    ssh_session session = ssh_new();
    if (session == NULL) return NULL;

    // SSH Session Options
    ssh_options_set(session, SSH_OPTIONS_HOST, host->ip);
    ssh_options_set(session, SSH_OPTIONS_PORT, &host->port);
    ssh_options_set(session, SSH_OPTIONS_USER, host->username);

    // Connect
    if (ssh_connect(session) != SSH_OK) {
        ssh_free(session);
        return NULL;
    }

    // Authenticate
    if (ssh_userauth_password(session, NULL, host->password) != SSH_AUTH_SUCCESS) {
        ssh_disconnect(session);
        ssh_free(session);
        return NULL;
    }

    // Create channel
    ssh_channel channel = ssh_channel_new(session);
    if (ssh_channel_open_session(channel) != SSH_OK) {
        ssh_channel_free(channel);
        ssh_disconnect(session);
        ssh_free(session);
        return NULL;
    }
    
    // Execute command: pid, user, %cpu, %mem, command (no header)
    const char *cmd = "ps -e -o pid,user,%cpu,%mem,comm --no-headers";
    if (ssh_channel_request_exec(channel, cmd) != SSH_OK) {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        ssh_disconnect(session);
        ssh_free(session);
        return NULL;
    }

    char buffer[4096];
    int nbytes;
    process_t *remote_list = NULL, *last_proc = NULL;
    
    // Accumulate output (simplified reading loop)
    while ((nbytes = ssh_channel_read(channel, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[nbytes] = '\0';
        
        // Split buffer by lines
        char *line = strtok(buffer, "\n");
        while (line != NULL) {
            // Allocate new process node directly
            process_t *p = calloc(1, sizeof(process_t));
            
            if (p) {
                // Parse line
                if (sscanf(line, "%d %63s %f %f %255s", 
                           &p->pid, p->user, &p->cpu_percent, &p->memory_percent, p->command) == 5) {
                    
                    p->next = NULL;
                    if (!remote_list) { 
                        remote_list = p; 
                        last_proc = p; 
                    } else { 
                        last_proc->next = p; 
                        last_proc = p; 
                    }
                } else {
                    free(p);
                }
            }
            line = strtok(NULL, "\n");
        }
    }

    // Cleanup
    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    ssh_disconnect(session);
    ssh_free(session);

    return remote_list;
}

// --- 3. CLEANUP ---

void free_remote_hosts(remote_host_t *head) {
    while (head) {
        remote_host_t *tmp = head;
        head = head->next;
        free(tmp);
    }
}

// --- 4. SIGNALS (Optional Stub) ---
int send_remote_signal(remote_host_t *host, int pid, int sig) {
    // Requires opening a new SSH session to run "kill -SIG <PID>"
    // Leaving as stub for now to allow compilation
    return -1; 
}
