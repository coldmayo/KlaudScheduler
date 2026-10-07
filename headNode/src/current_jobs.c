// save all queued and running jobs to a JSON file

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/file.h>
#include "cJSON.h"
#include "../includes/utils.h"
#include "../includes/types.h"
#include "../includes/current_jobs.h"

// Possible job status: RUNNING, QUEUED, DONE

// once job starts running add the PID of the process
#include <signal.h>
#include <limits.h>

int set_job_pid(int id_, int pid_) {
    ConfigInfo *config = get_config_info();
    char file_path[PATH_MAX], job_id[16];
    snprintf(file_path, sizeof(file_path), "%s/jobs.json", config->data_dir);
    snprintf(job_id, sizeof(job_id), "%d", id_);

    int lk = lock_jobs();
    if (lk < 0) return -1;

    int rc = -1;
    cJSON *jobs = read_json(file_path);
    cJSON *job = NULL;

    cJSON_ArrayForEach(job, jobs) {
        cJSON *id = cJSON_GetObjectItem(job, "job_id");
        if (!cJSON_IsString(id) || strcmp(id->valuestring, job_id) != 0) continue;

        cJSON *pid = cJSON_GetObjectItem(job, "PID");
        if (pid) cJSON_SetNumberValue(pid, pid_);
        else     cJSON_AddNumberToObject(job, "PID", pid_);

        rc = save_json(file_path, jobs);
        break;
    }

    cJSON_Delete(jobs);
    unlock_jobs(lk);
    return rc;
}

int kill_job(int pid, bool force) {
    if (pid <= 0) return -1;
    return kill(-pid, force ? SIGKILL : SIGTERM);
}

int cancel_job(int id_, bool force) {
    ConfigInfo *config = get_config_info();
    char file_path[PATH_MAX], job_id[16];
    snprintf(file_path, sizeof(file_path), "%s/jobs.json", config->data_dir);
    snprintf(job_id, sizeof(job_id), "%d", id_);

    int lk = lock_jobs();
    if (lk < 0) return -1;

    int rc = -1;
    cJSON *jobs = read_json(file_path);

    cJSON *target = NULL, *job = NULL;
    cJSON_ArrayForEach(job, jobs) {
        cJSON *id = cJSON_GetObjectItem(job, "job_id");
        if (cJSON_IsString(id) && strcmp(id->valuestring, job_id) == 0) {
            target = job;
            break;
        }
    }

    if (!target) {
        fprintf(stderr, "Job %d not found\n", id_);
        goto out;
    }

    cJSON *status = cJSON_GetObjectItem(target, "status");
    cJSON *owner = cJSON_GetObjectItem(target, "user");
    struct passwd *pw = getpwuid(getuid());
  	if (!cJSON_IsString(owner) || !pw || strcmp(owner->valuestring, pw->pw_name) != 0) {
      	fprintf(stderr, "Job %d belongs to another user\n", id_);
      	goto out;
  	}
    const char *st = cJSON_IsString(status) ? status->valuestring : "";

    if (strcmp(st, "QUEUED") == 0) {
        cJSON_ReplaceItemInObject(target, "status", cJSON_CreateString("CANCELLED"));
        rc = save_json(file_path, jobs);

    } else if (strcmp(st, "RUNNING") == 0) {
        cJSON *pid = cJSON_GetObjectItem(target, "PID");
        int p = cJSON_IsNumber(pid) ? pid->valueint : 0;
        if (p <= 0) {
            fprintf(stderr, "Job %d has no PID yet, try again in a moment\n", id_);
            goto out;
        }
        // Mark CANCELLED before signalling so the cleanup path doesn't record DONE
        cJSON_ReplaceItemInObject(target, "status", cJSON_CreateString("CANCELLED"));
        if (save_json(file_path, jobs) == 0) {
            if (kill_job(p, force) == 0) rc = 0;
            else perror("kill");
        }

    } else if (strcmp(st, "CANCELLED") == 0) {
        fprintf(stderr, "Job %d already cancelled\n", id_);
    } else if (strcmp(st, "DONE") == 0) {
        fprintf(stderr, "Job %d already finished\n", id_);
    } else {
        fprintf(stderr, "Job %d has unexpected status '%s'\n", id_, st);
    }

    out:
    cJSON_Delete(jobs);
    unlock_jobs(lk);
    return rc;
}

// Higher the priority # the quicker it gets run
double get_priority(int time, int cpus, int id) {
    ConfigInfo * config;
    config = get_config_info();
    int pos = id - 1000;
    int ticket = rand() % 20;
	double weights[4] = {0.1, -1.0, -1.0, 1.0};

    if (!config->lottery) weights[3] = 0.0;
    if (!config->aging)   weights[0] = 0.0;

	// priority functions
	double prior = 0;
	printf("Priority system: %s\n", config->priority_type);
	if (strcmp(config->priority_type, "SJR") == 0) {
		prior = (time*weights[0]) + (cpus*weights[1]) + (ticket*weights[3]);
	} else if (strcmp(config->priority_type, "FIFO") == 0) {
    	prior = (time*weights[0]) + (pos*weights[2]) + (ticket*weights[3]);
	} else {
    	printf("Does not understand selected proirity system, assuming FIFO\n");
        prior = (time*weights[0]) + (pos*weights[2]) + (ticket*weights[3]);
	}

	free(config);
	return prior;
}

int gen_id(void) {

    ConfigInfo * config;
    config = get_config_info();

    char file_path[200];
    sprintf(file_path, "%s/jobs.json", config->data_dir);
    cJSON * jobs_array = read_json(file_path);
    if (cJSON_GetArraySize(jobs_array) == 0) {
        return 1000;
    }
    int ind;
    cJSON * job = NULL;
    cJSON * id;
    cJSON_ArrayForEach(job, jobs_array) {
		id = cJSON_GetObjectItem(job, "job_id");
    }

    ind = atoi(id->valuestring);
    return ind+1;
}

int chg_status(int id_) {

    ConfigInfo * config;
    config = get_config_info();

    char file_path[200];
    sprintf(file_path, "%s/jobs.json", config->data_dir);
    
	char job_id[5];
	sprintf(job_id, "%d", id_);
    pthread_mutex_lock(&file_lock);
    cJSON * jobs_array = read_json(file_path);

	cJSON * job = NULL;
	cJSON_ArrayForEach(job, jobs_array) {
		cJSON * id = cJSON_GetObjectItem(job, "job_id");
		cJSON * status = cJSON_GetObjectItem(job, "status");
                
		if (strcmp(id->valuestring, job_id) == 0 && strcmp(status->valuestring, "QUEUED") == 0) {
			cJSON_ReplaceItemInObject(job, "status", cJSON_CreateString("RUNNING"));
			break;
		} else if (strcmp(id->valuestring, job_id) == 0 && strcmp(status->valuestring, "RUNNING") == 0) {
			cJSON_ReplaceItemInObject(job, "status", cJSON_CreateString("DONE"));
			break;
		}
	}

    FILE * fp = fopen(file_path, "w");
    if (fp) {
        char *json_string = cJSON_Print(jobs_array);
        fprintf(fp, "%s", json_string);
        free(json_string);
        fclose(fp);
    }

    cJSON_Delete(jobs_array);
    pthread_mutex_unlock(&file_lock);

    return 0;
}

void update_time(double elapsed) {
    ConfigInfo * config;
    config = get_config_info();

    char file_path[200];
    pthread_mutex_lock(&file_lock);
    sprintf(file_path, "%s/jobs.json", config->data_dir);
    cJSON * jobs_array = read_json(file_path);
    cJSON *job = NULL;
        cJSON_ArrayForEach(job, jobs_array) {
            cJSON *status = cJSON_GetObjectItem(job, "status");
            cJSON *time = cJSON_GetObjectItem(job, "time");
            cJSON * id = cJSON_GetObjectItem(job, "job_id");
            cJSON *cpu = cJSON_GetObjectItemCaseSensitive(job, "resources") ?
                         cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItem(job, "resources"), "cpu") :
                         NULL;

            if (!status || !status->valuestring || !time || !cpu) {
                continue;
            }

            if (strcmp(status->valuestring, "QUEUED") == 0) {
                double new_time = time->valuedouble + elapsed;
                cJSON_ReplaceItemInObject(job, "time", cJSON_CreateNumber(new_time));

                double new_prior = get_priority(new_time, cpu->valueint, id->valueint);
                cJSON_ReplaceItemInObject(job, "priority", cJSON_CreateNumber(new_prior));
            }
        }
    FILE * fp = fopen(file_path, "w");
    if (fp) {
        char *json_string = cJSON_Print(jobs_array);
        fprintf(fp, "%s", json_string);
        free(json_string);
        fclose(fp);
    }

    cJSON_Delete(jobs_array);
    pthread_mutex_unlock(&file_lock);
}

int clear_queue () {

    ConfigInfo * config;
    config = get_config_info();

    char file_path[200];
    sprintf(file_path, "%s/jobs.json", config->data_dir);

	FILE * fp = fopen(file_path, "w");
	if (fp == NULL) {
		return -1;
	}
	fclose(fp);
	return 0;
}

void save_job(int id, const char *comm, int cpu, const char *mem, int gpu, double priority, const char *out, const char *stat) {
    fflush(stdout);
    
    ConfigInfo * config;
    config = get_config_info();

    char file_path[200];
    sprintf(file_path, "%s/jobs.json", config->data_dir);
    
    cJSON *jobs_array = read_json(file_path);
    if (!jobs_array || !cJSON_IsArray(jobs_array)) {
        if (jobs_array) cJSON_Delete(jobs_array);
        jobs_array = cJSON_CreateArray();
    }
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);

    cJSON *job = cJSON_CreateObject();
    cJSON_AddStringToObject(job, "job_id", cJSON_Print(cJSON_CreateNumber(id)));
    cJSON_AddStringToObject(job, "user", pw->pw_name);
    cJSON_AddStringToObject(job, "command", comm);
    cJSON_AddNumberToObject(job, "PID", -1);
    cJSON *resources = cJSON_CreateObject();
    cJSON_AddNumberToObject(resources, "cpu", cpu);
    cJSON_AddStringToObject(resources, "memory", mem);
    cJSON_AddNumberToObject(resources, "gpu", gpu);
    cJSON_AddItemToObject(job, "resources", resources);

    cJSON_AddNumberToObject(job, "priority", priority);
    cJSON_AddNumberToObject(job, "time", 0);
    cJSON_AddStringToObject(job, "output", out);
    cJSON_AddStringToObject(job, "status", stat);

    cJSON_AddItemToArray(jobs_array, job);

    FILE * fp = fopen(file_path, "w");
    if (fp) {
        char *json_string = cJSON_Print(jobs_array);
        fprintf(fp, "%s", json_string);
        free(json_string);
        fclose(fp);
    }

    cJSON_Delete(jobs_array);
}
