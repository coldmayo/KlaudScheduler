#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "cJSON.h"
#include "../includes/utils.h"

const char *args[4] = {"--ID", "--num_cores", "--num_gpus", "--status"};

int check_args(const char * arg) {
	char temp[100];
	strncpy(temp, arg, sizeof(temp)-1);
	temp[sizeof(temp)-1] = '\0';

	char * token = strtok(temp, "=");
	if (!token) {
		return -1;
	}

	for (int i = 0; i < 4; i++) {
		if (strcmp(token, args[i]) == 0) {
			return i;
		}
	}

	return -1;
}

void filter(int index, char *slider, cJSON *jobs) {
    cJSON *job = NULL;
    for (int i = cJSON_GetArraySize(jobs) - 1; i >= 0; i--) {
        job = cJSON_GetArrayItem(jobs, i);
        cJSON *id = cJSON_GetObjectItem(job, "job_id");
        cJSON *status = cJSON_GetObjectItem(job, "status");
        cJSON *resource = cJSON_GetObjectItem(job, "resources");
        cJSON *gpu = cJSON_GetObjectItem(resource, "gpu");
        cJSON *cpu = cJSON_GetObjectItem(resource, "cpu");
        bool toDel = false;

        switch (index) {
            case 0:   // --ID
                if (strcmp(id->valuestring, slider) != 0) {
                    toDel = true;
                }
                break;
            case 1:   // --num_cores
                if (cpu->valueint != atoi(slider)) {
                    toDel = true;
                }
                break;
            case 2:   // --num_gpus
                if (gpu->valueint != atoi(slider)) {
                    toDel = true;
                }
                break;
            case 3:   // --status
                if (strcmp(status->valuestring, slider) != 0) {
                    toDel = true;
                }
                break;
            default:
                printf("Unknown index: %d\n", index);
        }

        if (toDel) {
            cJSON_DeleteItemFromArray(jobs, i);
        }
    }
}

int show_info(int argc, char ** argv) {
    ConfigInfo * config;
    config = get_config_info();

    char file_path[200];
    sprintf(file_path, "%s/jobs.json", config->dir);
    cJSON * jobs = read_json(file_path);

    // Filter out undesireables
	for (int i = 1; i < argc; i++) {
		int index = check_args(argv[i]);
		char * value = strchr(argv[i], '=');
		if (index >= 0 && value) {
			filter(index, value+1, jobs);
		}
	}

	// print out the leftovers
	cJSON * job = NULL;
	cJSON_ArrayForEach(job, jobs) {
    	cJSON * id = cJSON_GetObjectItem(job, "job_id");
		cJSON * status = cJSON_GetObjectItem(job, "status");
		cJSON * resource = cJSON_GetObjectItem(job, "resources");
		cJSON * gpu = cJSON_GetObjectItem(resource, "gpu");
		cJSON * cpu = cJSON_GetObjectItem(resource, "cpu");
		cJSON * command = cJSON_GetObjectItem(job, "command");
		
		printf(
			"Job ID: %s\n"
			"Status: %s\n"
			"CPU num: %d\n"
			"GPU num: %d\n"
			"Command: %s\n",
			id->valuestring,
			status->valuestring,
			cpu->valueint,
			gpu->valueint,
			command->valuestring
		);
	}
	
	return 0;

}

int main(int argc, char ** argv) {
	if (argc == 1) {
    	printf(
        	"Example Usage:\n"
        	"1. ktrack --ID=<job id>\n"
        	"2. ktrack --status=<job status> --num_cores=<# of cores>\n"
        	"where filter can be: DONE, QUEUED, and RUNNING\n"
    	);
    	return EXIT_FAILURE;
	}

	show_info(argc, argv);
}
