#ifndef gen_nodes_list_h_INCLUDED
#define gen_nodes_list_h_INCLUDED

#include <cjson/cJSON.h>

cJSON * check_nodes(const char * ip);
void update_status(int core, char * host);

#endif // gen_nodes_list_h_INCLUDED
