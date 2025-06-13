#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "cJSON.h"
#include "../includes/utils.h"
#include "../includes/node_status.h"
#include "../includes/types.h"

NODEINFO * node_info(char * hostname) {
	char cmd[200];
	char file_cpu[50];
	char file_ram[50];
	char * conts;

	sprintf(file_cpu, "cpu_%s.txt", hostname);
	sprintf(file_ram, "ram_%s.txt", hostname);

	sprintf(cmd, "ssh master@%s \"cat /proc/stat\" > %s && ssh master@%s \"cat /proc/meminfo\" > %s", hostname, file_cpu, hostname, file_ram);

	system(cmd);

	NODEINFO * info = malloc(sizeof(NODEINFO));
	memset(info, 0, sizeof(NODEINFO));

	// Find cpu/node usage

	// cpu usage
	conts = read_file(file_cpu);
	int user, nice, system, idle, iowait, irq, softirq;
	if (sscanf(conts, "cpu  %d %d %d %d %d %d %d", &user, &nice, &system, &idle, &iowait, &irq, &softirq)) {
    	int total = user + nice + system + idle + iowait + irq + softirq;
        int used = total - idle - iowait;
		info->cpuUse = (used*100.0)/total;
	}

	// per core usage
	char *ptr = conts;
    int core = 0;
    while ((ptr = strstr(ptr, "cpu")) != NULL && core < 12) {
        if (sscanf(ptr, "cpu%d %d %d %d %d %d %d %d", &core, &user, &nice, &system, &idle, &iowait, &irq, &softirq) == 8) {
            int total = user + nice + system + idle + iowait + irq + softirq;
            int used = total - idle - iowait;
            info->coreUse[core] = (used * 100.0) / total;
            core++;
        }
        ptr++;
    }
    info->num_cores = core;

	// Find ram usage
	char * cont2 = read_file(file_ram);
	int memTotal, memFree, buffers, cached, SReclaim, Shmem;
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

    info->ram_usage = (memTotal - memFree - buffers - cached - SReclaim - Shmem)/memTotal;

    free(cont2);
    free(conts);
	return info;
}

void updateNodeHealth() {
	NODEINFO * node_i = malloc(sizeof(NODEINFO));
	FILE * fp = fopen("nodes.json", "r");

	fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    if (file_size <= 0) {
        fclose(fp);
        return;
    }

    char *buffer = (char *)malloc(file_size + 1);
    fread(buffer, 1, file_size, fp);
    buffer[file_size] = '\0';
    fclose(fp);

    cJSON * node_array = cJSON_Parse(buffer);
    free(buffer);

	int i = 0;
    cJSON * node = NULL;
    cJSON_ArrayForEach(node, node_array) {
		cJSON * host = cJSON_GetObjectItem(node, "hostname");
		char * name = host->valuestring;
		node_i = node_info(name);
		i++;
    }

	FILE * nodeHFile = fopen("node_status.txt", "a");

	time_t current_time;
	struct tm *local_time;
	current_time = time(&current_time);
	local_time = localtime(&current_time);

	char info[300];
	char coreUse[150];
	
	for (int j = 0; j < node_i->num_cores; j++) {
		char ind_use[40];
		sprintf(ind_use, "Core slot %d usage: %f\n", j, node_i->coreUse[j]);
		strcat(coreUse, ind_use);
	}
	
	sprintf(info,"\nTime: %s\nInfo:\nCPU Use: %f\nCore Usage: %s\nRAM Usage: %f\n", asctime(local_time), node_i->cpuUse, coreUse, node_i->ram_usage);

	fputs(info, nodeHFile);

	fclose(nodeHFile);

	cJSON_Delete(node_array);
}
