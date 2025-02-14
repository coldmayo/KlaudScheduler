#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "current_jobs.h"

typedef struct {
    int cores;
    char *command;
} E_Job;

const char *list[2] = {"--num_cores", "--command"};

int check_args(const char *arg) {
    char temp[100];
    strncpy(temp, arg, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';

    char *token = strtok(temp, "=");
    if (!token) {
        return -1;
    }

    for (int i = 0; i < 2; i++) {
        if (strcmp(token, list[i]) == 0) {
            return i;
        }
    }
    return -1;
}

void get_info(int argc, char *argv[], E_Job *job) {
    job->command = malloc(256);
    if (!job->command) {
        printf("Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; i++) {
        int index = check_args(argv[i]);

        if (index == 0) {
            char *value = strchr(argv[i], '=');
            if (value) {
                job->cores = atoi(value + 1);
            }
        } else if (index == 1) {
            char *value = strchr(argv[i], '=');
            if (value) {
                strncpy(job->command, value + 1, 255);
                job->command[255] = '\0';
            }
        } else {
            printf("Unknown argument: %s\n", argv[i]);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: ./main --num_cores=<num> --command=\"<cmd>\"\n");
        return EXIT_FAILURE;
    }

    E_Job job = {0, NULL};
    get_info(argc, argv, &job);

    printf("Cores: %d\n", job.cores);
    printf("Command: %s\n", job.command);

	save_job(gen_id(), job.command, job.cores, "idk", 0, 1, "outfile.out", "QUEUED");

    free(job.command);
    return 0;
}
