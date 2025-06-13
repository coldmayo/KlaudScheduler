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
    FILE * fp = fopen("jobs.json", "r");
    if (!fp) {
        printf("Failed to open jobs.json\n");
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    if (file_size <= 0) {
        fclose(fp);
        printf("Empty jobs file\n");
        return NULL;
    }

    char *buffer = (char *)malloc(file_size + 1);
    size_t read = fread(buffer, 1, file_size, fp);
    if (read != file_size) {
        free(buffer);
        fclose(fp);
        printf("File read error\n");
        return NULL;
    }
    buffer[file_size] = '\0';
    fclose(fp);

    cJSON * jobs_array = cJSON_Parse(buffer);
    free(buffer);
    if (!jobs_array) {
        printf("Failed to parse JSON\n");
        return NULL;
    }

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
            int prior = priority->valueint;
            printf("QUEUED job found with priority %d\n", prior);

            if (prior > biggest) {
                printf("New highest priority: %d\n", prior);
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

void clean_up(cJSON * job, CPUout * c) {

    cJSON * id = cJSON_GetObjectItem(job, "job_id");
    printf("change job %s status to DONE\n", id->valuestring);
    fflush(stdout);
    chg_status(atoi(id->valuestring));

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
    cJSON * job = input->job;
    printf("Starting Execution\n");
    fflush(stdout);
	//pthread_mutex_lock(&lock);
	char cmd[300];
	int i = 0;

	cJSON * run = cJSON_GetObjectItem(job, "command");
	cJSON * id = cJSON_GetObjectItem(job, "job_id");
	gen_rankfile(atoi(id->valuestring), c);
	// rankfile format: rank <rank_id>=<hostname> slot=<core_binding>
	// make rankfile for every task it seems
	printf("%s\n", hostlist(c, atoi(id->valuestring)));
	fflush(stdout);
	snprintf(cmd, sizeof(cmd), "mpirun --host %s --map-by rankfile:file=%s_rankfile.txt %s 2>&1 | tee %s_output.txt", hostlist(c, atoi(id->valuestring)), id->valuestring, run->valuestring, id->valuestring);
	printf("%s\n", cmd);
	fflush(stdout);
	i = 0;
	while (c->hosts[i] != NULL) {
		update_status(c->cpu_cores[i], c->hosts[i]);
		i++;
	}
	chg_status(atoi(id->valuestring));
	//pthread_mutex_unlock(&lock);

	system(cmd);
    printf("Finished!\n");
	clean_up(job, c);
	return NULL;
}

void cpu_avail(int cpu_num, CPUout * c) {
    memset(c, 0, sizeof(CPUout));
    int * cpus_needed = malloc(sizeof(int) * cpu_num);
    if (!cpus_needed) {
        c->free = false;
        return;
    }

    FILE * fp = fopen("nodes.json", "r");
    if (!fp) {
        c->free = false;
        free(cpus_needed);
        return;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    if (file_size <= 0) {
        fclose(fp);
        c->free = false;
        free(cpus_needed);
        return;
    }

    char *buffer = (char *)malloc(file_size + 1);
    fread(buffer, 1, file_size, fp);
    buffer[file_size] = '\0';
    fclose(fp);

    cJSON * node_array = cJSON_Parse(buffer);
    free(buffer);
    if (!node_array) {
        c->free = false;
        free(cpus_needed);
        return;
    }

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

// for now, only do CPU only jobs
int main() {
    int res;
    int i = 0;
    while (1) {
        if (i == 1000) {
            updateNodeHealth();
            i = 0;
        }
        
        pthread_mutex_lock(&lock);
        cJSON * job = find_job();
        if (!job) {
            //printf("No jobs");
            pthread_mutex_unlock(&lock);
            //sleep(1);
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
        CPUout c = {0};

        cpu_avail(cpuNum, &c);

        if (!c.free) {
            printf("Not avail\n");
            fflush(stdout);
            pthread_cond_wait(&cpu_available, &lock);
        }

		// Make a seperate thread for 
        printf("Found %d core(s) for job %s\n", cpuNum, job_id->valuestring);
        fflush(stdout);
        pthread_t thread;
        EXECUTE* input = malloc(sizeof(EXECUTE));
        input->job = job;
        input->c = malloc(sizeof(CPUout));
        *(input->c) = c;
        res = pthread_create(&thread, NULL, execute_job, input);
        if (res != 0) {
            printf("failed to make pthread");
            fflush(stdout);
        }
        //execute_job(job, &c);
        pthread_mutex_unlock(&lock);
        i++;
    }
    return 0;
}
