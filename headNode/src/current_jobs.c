// save all queued and running jobs to a JSON file

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "cJSON.h"
#include "../includes/utils.h"
#include "../includes/types.h"

// Possible job status: RUNNING, QUEUED, DONE

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
    sprintf(file_path, "%s/jobs.json", config->dir);
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
    sprintf(file_path, "%s/jobs.json", config->dir);
    
	char job_id[5];
	sprintf(job_id, "%d", id_);
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

	return 0;
}

void update_time(double elapsed) {
    ConfigInfo * config;
    config = get_config_info();

    char file_path[200];
    sprintf(file_path, "%s/jobs.json", config->dir);
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
}

int clear_queue () {

    ConfigInfo * config;
    config = get_config_info();

    char file_path[200];
    sprintf(file_path, "%s/jobs.json", config->dir);

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
    sprintf(file_path, "%s/jobs.json", config->dir);
    
    cJSON *jobs_array = read_json(file_path);
    if (!jobs_array || !cJSON_IsArray(jobs_array)) {
        if (jobs_array) cJSON_Delete(jobs_array);
        jobs_array = cJSON_CreateArray();
    }

    cJSON *job = cJSON_CreateObject();
    cJSON_AddStringToObject(job, "job_id", cJSON_Print(cJSON_CreateNumber(id)));
    cJSON_AddStringToObject(job, "command", comm);
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
