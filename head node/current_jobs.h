#ifndef current_jobs_h_INCLUDED
#define current_jobs_h_INCLUDED

int chg_status(int id);
int clear_queue(void);
void save_job(int id, const char *comm, int cpu, const char *mem, int gpu, int priority, const char *out, const char *stat);
int gen_id();

#endif // current_jobs_h_INCLUDED
