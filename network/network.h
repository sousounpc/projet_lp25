#ifndef NETWORK_H
#define NETWORK_H

#include "manager.h"



int network_test_remote_targets(remote_target_t *targets);
int network_fetch_remote_processes(remote_target_t *targets);

#endif
