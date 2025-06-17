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

NODEINFO * node_info(char * addr) {
	char cmd[200];
	char file_cpu[50];
	char file_ram[50];
	char * conts;
	char * hostname = ip_alias(addr);

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
                //printf("Core %d: %.2f usage\n", cnt, info->coreUse[cnt]);
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

    time_t current_time;
    struct tm *local_time;
    current_time = time(&current_time);
    local_time = localtime(&current_time);
	
    FILE * nodeHFile = fopen("node_status.txt", "a");
    fprintf(nodeHFile, "\n===== Node(s) Health update: %s =====\n", asctime(local_time));

    cJSON * node_array = cJSON_Parse(buffer);
    free(buffer);

    cJSON * node = NULL;
    cJSON_ArrayForEach(node, node_array) {
		cJSON * host = cJSON_GetObjectItem(node, "hostname");
		char * name = host->valuestring;
		//printf("Collecting Data for %s\n", name);
		NODEINFO * node_i = node_info(name);
		//printf("Finished getting data\n");
		
	        char coreUse[150] = "";
	
	        for (int j = 0; j < node_i->num_cores; j++) {
		    char ind_use[40];
		    sprintf(ind_use, "Core slot %d usage: %.2f\n", j, node_i->coreUse[j]);
		    strcat(coreUse, ind_use);
	        }
	        
	        fprintf(nodeHFile,"\nNode: %s\nInfo:\nCPU Use: %.2f\nCore Usage:\n%s\nRAM Usage: %.2f\n", name, node_i->cpuUse, coreUse, node_i->ram_usage*100.0);
	        
	        free(node_i);
    }

	fclose(nodeHFile);
	cJSON_Delete(node_array);
	//printf("Node Health Updated\n");
}
