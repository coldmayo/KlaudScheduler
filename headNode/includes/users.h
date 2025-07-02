#ifndef users_h_INCLUDED
#define users_h_INCLUDED

void save_user(char * username, char * group_name);
void update_CPUt(char * username, double elapsed);
double get_CPU_time(char * username);
bool is_user_saved(char * username); 
void update_group_times(char *group_name, double elapsed);
void populate_groups();
double get_group_cpu_time(char * group_name);

#endif // users_h_INCLUDED
