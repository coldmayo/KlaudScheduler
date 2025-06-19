#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
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


E_Job * read_klaud_file(char * file_name) {
	E_Job * job_info = malloc(sizeof(E_Job));
	job_info->outfile = malloc(256);
	job_info->command = malloc(256);
	char * file_conts = read_file(file_name);

	// Get some job info
	char * line = strtok(file_conts, "\n");
	while (line) {
		sscanf(line, "#KLAUD --outfile=\"%[^\"]", job_info->outfile);
		sscanf(line, "#KLAUD --num_cores=%d", &job_info->cores);
		line = strtok(NULL, "\n");
	}

	//printf("%s %d\n", job_info->outfile, job_info->cores);
	sprintf(job_info->command, "chmod +x %s && ./%s", file_name, file_name);
	//printf("%s %s %d\n", job_info->outfile, job_info->command, job_info->cores);
	return job_info;
}
