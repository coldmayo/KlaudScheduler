#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "node_status.h"

char * read_file(char * file_name) {
	FILE * f;
	f = fopen(file_name, "rb");
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	rewind(f);
	char * str = malloc(size+1);
	size_t read_sz = fread(str, 1, size, f);
	str[size] = '\0';
	fclose(f);
	return str;
}

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
