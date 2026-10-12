#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "cJSON.h"
#include "../includes/utils.h"
#include "../includes/node_status.h"
#include "../includes/types.h"
#include "../includes/utils.h"


CLUSTERINFO * cluster_info(void) {
	ConfigInfo *config = get_config_info();
	if (config == NULL) {
		return NULL;
	}

	char file_path[150];
	snprintf(file_path, sizeof(file_path), "%s/nodes.json",
			 config->data_dir);

	pthread_mutex_lock(&file_lock);
	cJSON *root = read_json(file_path);
	pthread_mutex_unlock(&file_lock);

	if (root == NULL) {
		return NULL;
	}

	cJSON *node_array = cJSON_GetObjectItemCaseSensitive(root, "nodes");
	if (!cJSON_IsArray(node_array)) {
		cJSON_Delete(root);
		return NULL;
	}

	CLUSTERINFO *info = calloc(1, sizeof(*info));
	if (info == NULL) {
		cJSON_Delete(root);
		return NULL;
	}

	cJSON *node = NULL;

	cJSON_ArrayForEach(node, node_array) {
		if (info->num_nodes >= MAX_CORES) {
			break;
		}

		cJSON *cpu_object =
		cJSON_GetObjectItemCaseSensitive(node, "cpus");

		if (!cJSON_IsObject(cpu_object)) {
			continue;
		}

		int core_count = 0;
		cJSON *cpu = NULL;

		cJSON_ArrayForEach(cpu, cpu_object) {
			core_count++;
		}

		info->core_nums[info->num_nodes] = core_count;
		info->num_nodes++;
	}

	cJSON_Delete(root);
	return info;
}


NODEINFO * node_info(const char * addr) {
	char cmd[200];
	char file_cpu[50];
	char file_ram[50];
	char * conts;
	char * hostname = ip_alias(addr);

	sprintf(file_cpu, "cpu_%s.txt", hostname);
	sprintf(file_ram, "ram_%s.txt", hostname);

	sprintf(cmd, "ssh $USER@%s \"cat /proc/stat\" > %s && ssh $USER@%s \"cat /proc/meminfo\" > %s", hostname, file_cpu, hostname, file_ram);

	system(cmd);

	NODEINFO * info = malloc(sizeof(NODEINFO));
	memset(info, 0, sizeof(NODEINFO));

	// Find cpu/node usage

	// cpu usage
	conts = read_file(file_cpu);
	int user, nice, system, idle, iowait, irq, softirq;
	if (sscanf(conts, "cpu  %d %d %d %d %d %d %d", &user, &nice, &system, &idle, &iowait, &irq, &softirq) == 7) {
    	    int total = user + nice + system + idle + iowait + irq + softirq;
    	    if (total > 0.0) {
    	        int used = total - idle - iowait;
	        info->cpuUse = (used*100.0)/total;
    	    } else {
    	        info->cpuUse = 0.0;
    	    }
        
	} else {
	    printf("Could not find CPU stats for %s\n", hostname);
	}

	// per core usage
	char *ptr = conts;
        int cnt = 0;
    while ((ptr = strstr(ptr, "cpu")) != NULL) {
        int core;
        if (sscanf(ptr, "cpu%d %d %d %d %d %d %d %d", &core, &user, &nice, &system, &idle, &iowait, &irq, &softirq) == 8) {
            int total = user + nice + system + idle + iowait + irq + softirq;
            if (total > 0.0) {
                int used = total - idle - iowait;
                info->coreUse[cnt] = (used * 100.0) / total;
            } else {
                info->coreUse[cnt] = 0.0;
            }
            cnt++;
            
        }
        ptr++;
    }
    info->num_cores = cnt;
    free(conts);

	// Find ram usage
	char * cont2 = read_file(file_ram);
	int memTotal = 0, memFree= 0, buffers= 0, cached= 0, SReclaim= 0, Shmem= 0;
	char * line = strtok(cont2, "\n");
	while (line) {
		sscanf(line, "MemTotal: %d kB", &memTotal);
    	sscanf(line, "MemFree: %d kB", &memFree);
    	sscanf(line, "Buffers: %d kB", &buffers);
    	sscanf(line, "Cached: %d kB", &cached);
    	sscanf(line, "SReclaimable: %d kB", &SReclaim);
    	sscanf(line, "Shmem: %d kB", &Shmem);
    	line = strtok(NULL, "\n");
	}
	
    if (memTotal > 0) {
        info->ram_usage = (double)(memTotal - memFree - buffers - cached - SReclaim - Shmem)/memTotal;
    } else {
        info->ram_usage = 0.0;
    }

    free(cont2);
    remove(file_cpu);
    remove(file_ram);
    return info;
}

void updateNodeHealth() {
	ConfigInfo * config = get_config_info();
	char file_path[150];

	// Hold the lock only while reading the file, not during ssh
	snprintf(file_path, sizeof(file_path), "%s/nodes.json", config->data_dir);
	pthread_mutex_lock(&file_lock);
	cJSON * root = read_json(file_path);
	pthread_mutex_unlock(&file_lock);

	cJSON * node_array = cJSON_IsObject(root) ? cJSON_GetObjectItem(root, "nodes") : NULL;
	if (!cJSON_IsArray(node_array)) {
		cJSON_Delete(root);
		return;
	}

	snprintf(file_path, sizeof(file_path), "%s/node_status.txt", config->dir);
	FILE * out = fopen(file_path, "a");
	if (!out) {
		cJSON_Delete(root);
		return;
	}

	time_t now = time(NULL);
	struct tm tmv;
	char stamp[64];
	localtime_r(&now, &tmv);
	strftime(stamp, sizeof(stamp), "%a %b %d %H:%M:%S %Y", &tmv);
	fprintf(out, "\n===== Node(s) Health update: %s =====\n", stamp);

	cJSON * node = NULL;
	cJSON_ArrayForEach(node, node_array) {
		cJSON * host = cJSON_GetObjectItem(node, "hostname");
		if (!cJSON_IsString(host) || !host->valuestring) continue;

		NODEINFO * ni = node_info(host->valuestring);
		if (!ni) continue;

		fprintf(out, "\nNode: %s\nInfo:\nCPU Use: %.2f\nCore Usage:\n", host->valuestring, ni->cpuUse);
		for (int j = 0; j < ni->num_cores; j++) {
			fprintf(out, "Core slot %d usage: %.2f\n", j, ni->coreUse[j]);
		}
		fprintf(out, "\nRAM Usage: %.2f\n", ni->ram_usage * 100.0);
		free(ni);
	}

	fclose(out);
	cJSON_Delete(root);
}


HEALTH *updateNodeHealth_TUI(void) {
	ConfigInfo *config = get_config_info();
	if (config == NULL) {
		return NULL;
	}

	char file_path[150];

	snprintf(file_path, sizeof(file_path), "%s/nodes.json", config->data_dir);

	pthread_mutex_lock(&file_lock);
	cJSON *root = read_json(file_path);
	pthread_mutex_unlock(&file_lock);

	if (root == NULL) {
		return NULL;
	}

	cJSON *node_array =
	cJSON_IsObject(root)
	? cJSON_GetObjectItemCaseSensitive(root, "nodes")
	: NULL;

	if (!cJSON_IsArray(node_array)) {
		cJSON_Delete(root);
		return NULL;
	}

	HEALTH *health = calloc(1, sizeof(*health));
	if (health == NULL) {
		cJSON_Delete(root);
		return NULL;
	}

	cJSON *node = NULL;
	int nodes = 0;

	cJSON_ArrayForEach(node, node_array) {
		if (nodes >= MAX_NODES) {
			break;
		}

		cJSON *host =
		cJSON_GetObjectItemCaseSensitive(node, "hostname");

		if (!cJSON_IsString(host) || host->valuestring == NULL) {
			continue;
		}

		NODEINFO *ni = node_info(host->valuestring);
		if (ni == NULL) {
			continue;
		}

		health->tot_core[nodes] = ni->cpuUse;

		int core_count = ni->num_cores;
		if (core_count > MAX_CORES) {
			core_count = MAX_CORES;
		}

		for (int j = 0; j < core_count; j++) {
			health->cores[nodes][j] = ni->coreUse[j];
		}

		health->RAM[nodes] = ni->ram_usage;

		free(ni);
		nodes++;
	}

	health->num_nodes = nodes;

	cJSON_Delete(root);
	return health;
}
