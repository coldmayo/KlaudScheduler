// save all queued and running jobs to a JSON file

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

// Possible job status: RUNNING, QUEUED, DONE

int gen_id(void) {
	FILE * fp = fopen("jobs.json", "r");
	cJSON * jobs_array;
	if (fp == NULL) {
		return -1;
	}

	fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    if (file_size > 0) {
        char *buffer = (char *)malloc(file_size + 1);
        fread(buffer, 1, file_size, fp);
        buffer[file_size] = '\0';

        jobs_array = cJSON_Parse(buffer);
        free(buffer);
    } else {
        fclose(fp);
		return 300;
    }
    
    fclose(fp);
	int ind;
    cJSON * job = NULL;
    cJSON * id;
    cJSON_ArrayForEach(job, jobs_array) {
		id = cJSON_GetObjectItem(job, "job_id");
    }

    ind = atoi(id->valuestring);
    return ind+1;
}

int chg_status(int id) {
	FILE * fp = fopen("jobs.json", "r");
	char job_id[5];
	cJSON * jobs_array;
	sprintf(job_id, "%d", id);
	
	if (fp == NULL) {
		return -1;
	}

	fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    if (file_size > 0) {
        char *buffer = (char *)malloc(file_size + 1);
        fread(buffer, 1, file_size, fp);
        buffer[file_size] = '\0';

        jobs_array = cJSON_Parse(buffer);
        free(buffer);
    }
    fclose(fp);	

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

	char *updated_json = cJSON_Print(jobs_array);
	fp = fopen("jobs.json", "w");
    if (fp) {
        char *json_string = cJSON_Print(jobs_array);
        fprintf(fp, "%s", json_string);
        free(json_string);
        fclose(fp);
    }

    cJSON_Delete(jobs_array);

	return 0;
}

int clear_queue () {
	FILE * fp = fopen("jobs.json", "w");
	if (fp == NULL) {
		return -1;
	}
	fclose(fp);
	return 0;
}

void save_job(int id, const char *comm, int cpu, const char *mem, int gpu, int priority, const char *out, const char *stat) {
    FILE *fp = fopen("jobs.json", "r");
    cJSON *jobs_array = NULL;

    if (fp) {
        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        rewind(fp);

        if (file_size > 0) {
            char *buffer = (char *)malloc(file_size + 1);
            fread(buffer, 1, file_size, fp);
            buffer[file_size] = '\0';

            jobs_array = cJSON_Parse(buffer);
            free(buffer);
        }
        fclose(fp);
    }

    if (!cJSON_IsArray(jobs_array)) {
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
    cJSON_AddStringToObject(job, "output", out);
    cJSON_AddStringToObject(job, "status", stat);

    cJSON_AddItemToArray(jobs_array, job);

    fp = fopen("jobs.json", "w");
    if (fp) {
        char *json_string = cJSON_Print(jobs_array);
        fprintf(fp, "%s", json_string);
        free(json_string);
        fclose(fp);
    }

    cJSON_Delete(jobs_array);
}
