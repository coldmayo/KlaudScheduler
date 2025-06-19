#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "../includes/current_jobs.h"
#include "../includes/types.h"
#include "../includes/klaud_files.h"

const char *list[4] = {"--num_cores", "--command", "--outfile", "--file"};

int check_args(const char *arg) {
    char temp[100];
    strncpy(temp, arg, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';

    char *token = strtok(temp, "=");
    if (!token) {
        return -1;
    }

    for (int i = 0; i < 4; i++) {
        if (strcmp(token, list[i]) == 0) {
            return i;
        }
    }
    return -1;
}

int get_info(int argc, char *argv[], E_Job *job) {
    job->command = malloc(256);
    job->outfile = malloc(256);
    char * klaud_file = malloc(256);
	
    if (!job->command) {
        printf("Memory allocation failed!\n");
        exit(EXIT_FAILURE);
        return -2;
    }

    for (int i = 1; i < argc; i++) {
        int index = check_args(argv[i]);
		char * value;
        if (index == 0) {
            value = strchr(argv[i], '=');
            if (value) {
                job->cores = atoi(value + 1);
            } else {
				return -1;
            }
        } else if (index == 1) {
            value = strchr(argv[i], '=');
            if (value) {
                strncpy(job->command, value + 1, 255);
                job->command[255] = '\0';
            } else {
				return -1;
            }
        } else if (index == 2) {
            value = strchr(argv[i], '=');
            if (value) {
                strncpy(job->outfile, value+1, 255);
                job->command[255] = '\0';
            } else {
				return -1;
            }
            
        } else if (index == 3) {
			value = strchr(argv[i], '=');
			if (value) {
				strncpy(klaud_file, value+1, 255);
				klaud_file[255] = '\0';
				E_Job * file_job = read_klaud_file(klaud_file);
				if (file_job) {
					job->cores = file_job->cores;
					if (file_job->command) {
						strncpy(job->command, file_job->command, 255);
						job->command[255] = '\0';
					}
					if (file_job->outfile) {
						strncpy(job->outfile, file_job->outfile, 255);
						job->outfile[255] = '\0';
					}
				}
				free(file_job);
				return 0;
			}
        } else {
            printf("Unknown argument: %s\n", argv[i]);
            return -1;
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
wrong:
        printf("Usage: ./main --num_cores=<num> --command=\"<command>\" --outfile=\"<output file>\"\n");
        return EXIT_FAILURE;
    }
    
    E_Job job = {0, NULL, NULL};
    int ret = get_info(argc, argv, &job);
	if (ret == -1) {
		goto wrong;
    } else if (ret == -2) {
		return EXIT_FAILURE;
    }

    // Some error handling
    if (job.cores == 0) {
        goto wrong;
    } else if (strcmp(job.command, "\0") == 0) {
        goto wrong;
    }
    
    printf("Cores: %d\n", job.cores);
    printf("Command: %s\n", job.command);
  
    save_job(gen_id(), job.command, job.cores, "idk", 0, get_priority(0, job.cores), job.outfile, "QUEUED");

    free(job.command);
    return 0;
}
