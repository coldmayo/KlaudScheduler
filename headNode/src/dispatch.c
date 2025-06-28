#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include "cJSON.h"
//#include <cjson/cJSON.h>
#include <stdbool.h>
#include <string.h>
#include "../includes/utils.h"
#include "../includes/current_jobs.h"
#include "../includes/types.h"
#include "../includes/nodes_list.h"
#include "../includes/node_status.h"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t file_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cpu_available = PTHREAD_COND_INITIALIZER;

char * hostlist(CPUout * c, int id) {
    char * hosts = malloc(70 * sizeof(char));
    hosts[0] = '\0';
    for (int i = 0; c->hosts[i] != NULL; i++) {
        char num[4];
        if (i == 0) {
            strcat(hosts, ip_alias(c->hosts[i]));
            strcat(hosts, ":");
            sprintf(num, "%d", cpu_ranks(ip_alias(c->hosts[i]), id));
            strcat(hosts, num);
        } else if (strcmp(c->hosts[i], c->hosts[i-1]) != 0) {
            strcat(hosts, ",");
            strcat(hosts, ip_alias(c->hosts[i]));
            strcat(hosts, ":");
            sprintf(num, "%d", cpu_ranks(ip_alias(c->hosts[i]), id));
            strcat(hosts, num);
        }
    }
    return hosts;
}

// for now, just doing jobs in submission order (First Come first Serve)
cJSON * find_job() {

    ConfigInfo * config;
    config = get_config_info();

    char file_path[200];
    //printf("%s\n", file_path);
    sprintf(file_path, "%s/jobs.json", config->dir);
    //printf("%s\n", file_path);
    cJSON * jobs_array = read_json(file_path);

    int biggest = 0;
    cJSON * biggest_job = NULL;
    cJSON * job = NULL;

    cJSON_ArrayForEach(job, jobs_array) {
        cJSON * status = cJSON_GetObjectItem(job, "status");
        cJSON * priority = cJSON_GetObjectItem(job, "priority");

        // Safe pointer checking
        if (!status || !priority || !status->valuestring) {
            printf("Skipping job - missing fields\n");
            continue;
        }

        //printf("Checking job: status=%s, priority=%d\n", status->valuestring, priority->valueint);

        if (strcmp(status->valuestring, "QUEUED") == 0) {
            double prior = priority->valuedouble;
            printf("QUEUED job found with priority %f\n", prior);

            if (prior > biggest) {
                printf("New highest priority: %f\n", prior);
                biggest = prior;
                if (biggest_job) {
                    cJSON_Delete(biggest_job);
                }
                biggest_job = cJSON_Duplicate(job, 1);
                if (!biggest_job) {
                    printf("Failed to duplicate job\n");
                }
            }
        }
    }

    //printf("Highest priority found: %d\n", biggest);
    cJSON_Delete(jobs_array);

    if (biggest == 0) {
        return NULL;
    }

    return biggest_job;
}

void clean_up(cJSON * job, CPUout * c, double time) {
    if (!job || !c) {
        printf("NULL JOB or CPUout in cleanup()\n");
    }
    cJSON * id = cJSON_GetObjectItem(job, "job_id");
    printf("change job %s status to DONE\n", id->valuestring);
    fflush(stdout);
    chg_status(atoi(id->valuestring));
    update_time(time);
    for (int i = 0; c->hosts[i] != NULL && i < 20; i++) {
        if (c->cpu_cores[i] == 0) continue;
        printf("Change core %d status from host %s\n", c->cpu_cores[i], c->hosts[i]);
        fflush(stdout);
        update_status(c->cpu_cores[i], c->hosts[i]);
    }
    printf("Cleaned up job %s\n", id->valuestring);
    fflush(stdout);
}

void * execute_job(void * args) {
    EXECUTE * input = (EXECUTE *) args;
    CPUout * c = input->c;
    time_t start = input->start;
    time_t end;
    char * hosts;
    printf("Starting Execution\n");
    fflush(stdout);
    char cmd[700];
    char rankfile_name[200];
    
    ConfigInfo * config;
    config = get_config_info();

    cJSON * job_copy = cJSON_Duplicate(input->job, 1);
    free(input);

    if (!job_copy) {
        printf("Failed to duplicate JSON info for job :(");
        fflush(stdout);
        goto clean_cpu;
    }
	cJSON * run = cJSON_GetObjectItem(job_copy, "command");
	cJSON * id = cJSON_GetObjectItem(job_copy, "job_id");
	cJSON * outf = cJSON_GetObjectItem(job_copy, "output");
	
	char outfile[255];
	if (strcmp("\0", outf->valuestring) == 0) {
	    sprintf(outfile, "%s_output.out", id->valuestring);
	} else {
	    sprintf(outfile, "%s", outf->valuestring);
	}
	
	gen_rankfile(atoi(id->valuestring), c);

	// rankfile format: rank <rank_id>=<hostname> slot=<core_binding>
	// make rankfile for every task it seems

    hosts = hostlist(c, atoi(id->valuestring));
	printf("Hosts: %s\n", hosts);
	fflush(stdout);
    sprintf(rankfile_name, "%s/%s_rankfile.txt", config->dir, id->valuestring);

	//snprintf(cmd, sizeof(cmd), "mpirun --host %s --map-by rankfile:file=%s_rankfile.txt ./%s 2>&1 | tee %s", hosts, id->valuestring, run->valuestring, outfile);
	snprintf(cmd, sizeof(cmd), "mpirun --host %s --map-by rankfile:file=%s ./%s > %s", hosts, rankfile_name, run->valuestring, outfile);
	//printf("%s\n", cmd);
	
    // run job
    
    if (system(cmd) != 0) {
        printf("Run failed!\n");
    }
    printf("Finished!\n");
    time(&end);
    pthread_mutex_lock(&lock);
    clean_up(job_copy, c, difftime(end, start));
    pthread_mutex_unlock(&lock);
    
    // cleaning
    
    remove(rankfile_name);
    cJSON_Delete(job_copy);
clean_cpu:
    //printf("print");
    fflush(stdout);
    for (int i = 0; c->hosts[i] != NULL && i < c->num_hosts; i++) {
        free(c->hosts[i]);
    }
    free(c->cpu_cores);
    free(c);
    
    pthread_cond_signal(&cpu_available);
    return NULL;
}

void cpu_avail(int cpu_num, CPUout * c) {
    memset(c, 0, sizeof(CPUout));
    int * cpus_needed = malloc(sizeof(int) * cpu_num);
    if (!cpus_needed) {
        c->free = false;
        return;
    }
    char file_path[200];
    
    ConfigInfo * config;
    config = get_config_info();
    
    sprintf(file_path, "%s/nodes.json", config->dir);
    
    cJSON * node_array = read_json(file_path);

    int cpu_av = 0;
    cJSON * node = NULL;
    cJSON_ArrayForEach(node, node_array) {
        cJSON * host = cJSON_GetObjectItem(node, "hostname");
        cJSON * cpus = cJSON_GetObjectItem(node, "cpus");

        if (!host || !cpus || !host->valuestring) continue;

        cJSON * cpu = NULL;
        cJSON * cpu_item = NULL;
        cJSON_ArrayForEach(cpu_item, cpus) {
            cpu = cpu_item;
            cJSON * core = cJSON_GetObjectItem(cpu, "core #");
            cJSON * status = cJSON_GetObjectItem(cpu, "avail");

            if (!core || !status || !core->valuestring || !status->valuestring) continue;

            if (strcmp(status->valuestring, "FREE") == 0) {
                cpus_needed[cpu_av] = atoi(core->valuestring);
                c->hosts[cpu_av] = strdup(host->valuestring);
                printf("%d %s\n", cpus_needed[cpu_av], c->hosts[cpu_av]);
                fflush(stdout);
                if (!c->hosts[cpu_av]) {
                    for (int i = 0; i < cpu_av; i++) {
                        free(c->hosts[i]);
                    }
                    free(cpus_needed);
                    cJSON_Delete(node_array);
                    c->free = false;
                    return;
                }
                cpu_av++;
                if (cpu_av >= cpu_num) break;
            }
        }
        if (cpu_av >= cpu_num) break;
    }

    cJSON_Delete(node_array);
    c->num_hosts = cpu_av;
    if (cpu_av >= cpu_num) {
        c->cpu_cores = cpus_needed;
        //printf("free");
        c->free = true;
    } else {
        for (int i = 0; i < cpu_av; i++) {
            free(c->hosts[i]);
        }
        free(cpus_needed);
        c->free = false;
    }
}

void * check_time(void * args) {
    time_t start, check;
    time(&start);
	while (1) {
	    time(&check);
    	    double time_diff = difftime(check, start);
    	    if (time_diff >= 10.0) {
        	//printf("It has been 5 sec\n");
        		updateNodeHealth();
        		start = 0;
        		check = 0;
        		time(&start);
    		}
	}
}

// for now, only do CPU only jobs
int main() {
    int other_res;
    int i = 0;
    time_t start;
    set_all_Free();
    pthread_t stat_thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    // Every 10 seconds track cpu health
    other_res = pthread_create(&stat_thread, NULL, check_time, NULL);
    if (other_res != 0) {
        printf("Could not create thread :(");
    }
    while (1) {
        time(&start);
        pthread_mutex_lock(&lock);
        cJSON * job = find_job();
        if (!job) {
            pthread_mutex_unlock(&lock);
            continue;
        }
        
        cJSON * job_id = cJSON_GetObjectItem(job, "job_id");
        printf("Found a job: %s\n", job_id->valuestring);
        fflush(stdout);
        cJSON * resources = cJSON_GetObjectItem(job, "resources");
        if (!resources) {
            pthread_mutex_unlock(&lock);
            printf("Error: Job %d missing resources field\n", job_id->valueint);
            continue;
        }

        cJSON * cpu_num = cJSON_GetObjectItem(resources, "cpu");
        if (!cpu_num) {
            printf("Error: Job %d missing cpu feild\n", job_id->valueint);
            pthread_mutex_unlock(&lock);
            continue;
        }

        int cpuNum = cpu_num->valueint;
        printf("# CPUs required by job %s are %d\n", job_id->valuestring, cpuNum);
        fflush(stdout);
        CPUout* c = malloc(sizeof(CPUout));
        cpu_avail(cpuNum, c);

        while (!c->free) {
            printf("Not avail\n");
            fflush(stdout);
            free(c);
            pthread_cond_wait(&cpu_available, &lock);
            
            c=malloc(sizeof(CPUout));
            cpu_avail(cpuNum, c);
        }

	// Make a separate thread for executing the job
	
        printf("Found %d core(s) for job %s\n", cpuNum, job_id->valuestring);
        fflush(stdout);

        // Make sure the job is set to running, we don't want the same job to happen multiple times
        cJSON * stat = cJSON_GetObjectItem(job, "status");
        printf("Changing status of job %d from %s to RUNNING\n", atoi(job_id->valuestring), stat->valuestring);
        chg_status(atoi(job_id->valuestring));
        // Set the cores that will be used to Occupied so other jobs wont take them while this job is running
        int j = 0;
        while (c->hosts[j] != NULL) {
            update_status(c->cpu_cores[j], c->hosts[j]);
            j++;
        }

        pthread_t thread;
        EXECUTE* input = malloc(sizeof(EXECUTE));
        input->job = cJSON_Duplicate(job, 1);
        input->c = malloc(sizeof(CPUout));
        input->c = c;
        input->start = start;

        if (pthread_create(&thread, NULL, execute_job, input) != 0) {
            printf("failed to make pthread");
            fflush(stdout);
            free(input->c);
            free(input);
            pthread_mutex_unlock(&lock);
            continue;
        }
        pthread_mutex_unlock(&lock);
        i++;
    }
    pthread_attr_destroy(&attr);
    return 0;
}
