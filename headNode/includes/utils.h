#ifndef utils_h_INCLUDED
#define utils_h_INCLUDED

#include <pthread.h>
#include "types.h"

extern pthread_mutex_t file_lock;

char * read_file(char * file_name);
char * ip_alias(char * ip);
void gen_rankfile(int id, CPUout * c);
int cpu_ranks(char * hostname, int id);
cJSON * read_json(char * filename);
ConfigInfo * get_config_info(void);
char ** get_ip_hosts();
int get_dispatch_count(void);
int save_json(const char *path, cJSON *root);
int lock_jobs(void);
void unlock_jobs(int fd);

#endif // utils_h_INCLUDED
