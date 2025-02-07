#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <cjson/cJSON.h>
#include <stdbool.h>
#include <string.h>
#include "current_jobs.h"
#include "nodes_list.h"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cpu_available = PTHREAD_COND_INITIALIZER;

typedef struct {
	int * cpu_cores;
	char * hosts[20];
	bool free;
} CPUout;

// for now, just doing jobs in submission order
cJSON * find_job() {
	FILE * fp = fopen("jobs.json", "r");
	if (!fp) return NULL;
	cJSON * jobs_array;
	cJSON * job = NULL;

	fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    if (file_size <= 0) {
        fclose(fp);
        return NULL;
    }
    
    char *buffer = (char *)malloc(file_size + 1);
    fread(buffer, 1, file_size, fp);
    buffer[file_size] = '\0';

    jobs_array = cJSON_Parse(buffer);
    free(buffer);
    fclose(fp);	

	int biggest = 0;
	cJSON * biggest_job = NULL;
	cJSON_ArrayForEach(job, jobs_array) {
		cJSON * id = cJSON_GetObjectItem(job, "job_id");
		cJSON * status = cJSON_GetObjectItem(job, "status");

		if (strcmp(status->valuestring, "QUEUED") == 0) {
			int current_id = atoi(id->valuestring);
			if (current_id > biggest) {
    			biggest = current_id;
				biggest_job = job;
			}
		}
	}

	cJSON_Delete(jobs_array);
	if (biggest == 0) {
		return NULL;
	}
	return biggest_job;
}

void clean_up(cJSON * job, CPUout * c) {
	pthread_mutex_lock(&lock);

	cJSON * id = cJSON_GetObjectItem(job, "job_id");
	chg_status(atoi(id->valuestring));

	int i = 0;

	while (c->hosts[i] != NULL) {
		update_status(c->cpu_cores[i], c->hosts[i]);
		i++;
	}

	pthread_mutex_unlock(&lock);
}

void execute_job(cJSON * job, CPUout * c) {
	pthread_mutex_lock(&lock);
	char cmd[300];
	int i = 0;
	char host_list[200] = "";
	char core_list[30] = "";
	int num_cores = 0;
	for (int i = 0; c->hosts[i] != NULL; i++) {
    	if (i != 0) {
			strncat(host_list, ", ", sizeof(host_list) - strlen(host_list) - 1); // strncat is "safer" or whatever...
    	}
		strncat(host_list, c->hosts[i], sizeof(host_list) - strlen(host_list) - 1);	
	}

	for (int i = 0; c->cpu_cores[i] != 0; i++) {
		if (i != 0) {
			strncat(core_list, ", ", sizeof(host_list) - strlen(host_list) - 1);
    	}
    	char core_str[10];
        sprintf(core_str, "%d", c->cpu_cores[i]);
		strcat(core_list, core_str);	
		i++;
	}
	num_cores = i;
	cJSON * run = cJSON_GetObjectItem(job, "command");
	cJSON * id = cJSON_GetObjectItem(job, "job_id");
	snprintf(cmd, sizeof(cmd), "mpirun --host %s%s -np %d %s", host_list, core_list, num_cores, run->valuestring);
	i = 0;
	while (c->hosts[i] != NULL) {
		update_status(c->cpu_cores[i], c->hosts[i]);
		i++;
	}
	chg_status(atoi(id->valuestring));
	pthread_mutex_unlock(&lock);

	system(cmd);

	clean_up(job, c);
}



void cpu_avail(int cpu_num, CPUout * c) {
	int * cpus_needed = malloc(sizeof(int) * cpu_num);
	char * hosts[20];
	FILE * fp = fopen("nodes.json", "r");
	char cpu_n[5];
	
	if (!fp) {
		c->free = false;
		free(cpus_needed);
		return;
	}
	
	cJSON * node_array = NULL;

	fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    if (file_size > 0) {
        char *buffer = (char *)malloc(file_size + 1);
        fread(buffer, 1, file_size, fp);
        buffer[file_size] = '\0';

        node_array = cJSON_Parse(buffer);
        free(buffer);
    }
    fclose(fp);	
    if (!node_array) {
		c->free = false;
		free(cpus_needed);
		return;
    }

    cJSON * node = NULL;
	int cpu_av = 0;

    cJSON_ArrayForEach(node, node_array) {
		cJSON * cpu = cJSON_GetObjectItem(node, "cpus");
		cJSON * host = cJSON_GetObjectItem(node, "hostname");
		cJSON * cpu_info = NULL;
		cJSON_ArrayForEach(cpu_info, cpu) {
			cJSON * core = cJSON_GetObjectItem(cpu_info, "core #");
			cJSON * status = cJSON_GetObjectItem(cpu_info, "avail");
			sprintf(cpu_n, "%d", cpu_num);
			if (strcmp(core->valuestring, cpu_n) == 0 && strcmp(status->valuestring, "FREE") == 0) {
				cpus_needed[cpu_av] = atoi(core->valuestring);
				strcpy(hosts[cpu_av], host->valuestring);
				cpu_av++;
				if (cpu_av >= cpu_num) break;
			}
		}
		if (cpu_av >= cpu_num) break;
    }
    cJSON_Delete(node_array);

	if (cpu_av >= cpu_num) {
		c->cpu_cores = cpus_needed;
    	c->free = true;
    	for (int i = 0; i < cpu_av; i++) {
			c->hosts[i] = hosts[i];
    	}
	} else {
    	free(cpus_needed);
    	for (int i = 0; i < cpu_av; i++) {
            free(hosts[i]);
        }
		c->cpu_cores = NULL;
		c->hosts[0] = NULL;
    	c->free = false;
	}

}

// for now, only do CPU only jobs
int main() {
    while (1) {
        pthread_mutex_lock(&lock);
        cJSON * job = find_job();
        if (!job) {
            pthread_mutex_unlock(&lock);
            continue;
        }

        cJSON * cpu_num = cJSON_GetObjectItem(job, "cpu");
        if (!cpu_num) {
            pthread_mutex_unlock(&lock);
            continue;
        }

        int cpuNum = atoi(cpu_num->valuestring);
        CPUout c;
        cpu_avail(cpuNum, &c);

        while (!c.free) {
            pthread_cond_wait(&cpu_available, &lock);
        }
		execute_job(job, &c);
        pthread_mutex_unlock(&lock);
    }
    return 0;
}
