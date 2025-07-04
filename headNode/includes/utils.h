#ifndef utils_h_INCLUDED
#define utils_h_INCLUDED

#include "types.h"

char * read_file(char * file_name);
char * ip_alias(char * ip);
void gen_rankfile(int id, CPUout * c);
int cpu_ranks(char * hostname, int id);
cJSON * read_json(char * filename);
ConfigInfo * get_config_info(void);
char ** get_ip_hosts();

#endif // utils_h_INCLUDED
