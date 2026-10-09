#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "../includes/utils.h"
#include "../includes/types.h"
#include "../includes/klaud_files.h"

/*
More about .klaud files:
Basically like .slurm files, used to submit jobs to the KlaudScheduler 
- which are also basically bash files also
Keep in mind the actual bash commands (like running the executable) will occur on all nodes

Here is a very basic example:

#!/bin/bash
#KLAUD --outfile="output.out"
#KLAUD --num_cores=3

./examples/hello_mpi

*/

static char *trim_whitespace(char *str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;

    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';

    return str;
}

E_Job * read_klaud_file(char * file_name) {
	E_Job * job_info = calloc(1, sizeof(E_Job));
        if (!job_info) return NULL;

        job_info->outfile = malloc(256);
        if (job_info->outfile) job_info->outfile[0] = '\0';

        char * file_conts = read_file(file_name);
        if (!file_conts) {
            free(job_info->outfile);
            free(job_info);
            return NULL;
        }
	
	char *running = file_conts;
        char *token;
	while ((token = strsep(&running, "\n")) != NULL) {
	        char *line = trim_whitespace(token);
	        if (line[0] == '\0') continue;
	        
		if (strncmp(line, "#KLAUD", 6) == 0) {
                    if (strstr(line, "--outfile")) {
                        sscanf(line, "#KLAUD --outfile=\"%255[^\"]\"", job_info->outfile);
                    }
                    if (strstr(line, "--num_cores")) {
                        sscanf(line, "#KLAUD --num_cores=%d", &job_info->cores);
                    }
                    continue;
                }
                
                if (line[0] == '#') continue;
                
                job_info->commands = realloc(job_info->commands, sizeof(char *) * (job_info->num_commands + 1));
                job_info->commands[job_info->num_commands] = strdup(line);
                job_info->num_commands++;
	}
	
	if (job_info->num_commands > 0) {
                size_t total_len = 0;
                for (size_t i = 0; i < job_info->num_commands; i++) {
                    total_len += strlen(job_info->commands[i]);
                }
                
                total_len += (job_info->num_commands - 1) * 3 + 1;

                job_info->command = malloc(total_len);
                if (job_info->command) {
                    job_info->command[0] = '\0';
                    for (size_t i = 0; i < job_info->num_commands; i++) {
                        if (i > 0) {
                            strcat(job_info->command, " ; ");
                        }
                        strcat(job_info->command, job_info->commands[i]);
                    }
                }
        } else {
            job_info->command = NULL;
        }

    free(file_conts);
    return job_info;
}
