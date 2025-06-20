#ifndef gen_nodes_list_h_INCLUDED
#define gen_nodes_list_h_INCLUDED

#include "../src/cJSON.h"
#include <stdbool.h>

cJSON * check_nodes(const char * ip, bool tcp);
void update_status(int core, char * host);

#endif // gen_nodes_list_h_INCLUDED
