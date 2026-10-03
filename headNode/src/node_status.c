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
