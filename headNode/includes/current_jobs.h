#ifndef current_jobs_h_INCLUDED
#define current_jobs_h_INCLUDED

int chg_status(int id);
int clear_queue(void);
void save_job(int id, const char *comm, int cpu, const char *mem, int gpu, double priority, const char *out, const char *stat);
int gen_id();
double get_priority(int time, int cpus, int id);
void update_time(double time);
int set_job_pid(int id_, int pid_);
int cancel_job(int id_, bool force);

#endif // current_jobs_h_INCLUDED
