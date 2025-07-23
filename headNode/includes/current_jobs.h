#ifndef current_jobs_h_INCLUDED
#define current_jobs_h_INCLUDED

int chg_status(int id);
int clear_queue(void);
void save_job(int id, const char *comm, int cpu, const char *mem, int gpu, double priority, const char *out, const char *stat);
int gen_id();
int get_priority(int time, int cpus, int id);
void update_time(double time);
char * get_status(int id);
void add_time_vals(int id, char * json_element);

#endif // current_jobs_h_INCLUDED
