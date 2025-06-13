#ifndef types_h_INCLUDED
#define types_h_INCLUDED

#include "../src/cJSON.h"
#define MAX_CORES 256

typedef struct {
	int * cpu_cores;
	char * hosts[20];
	bool free;
} CPUout;

typedef struct {
	cJSON * job;
	CPUout * c;
} EXECUTE;

typedef struct {
    int cores;
    char *command;
} E_Job;

typedef struct {
	float cpuUse;
	int num_cores;
	float coreUse[12];
	float ram_usage;
} NODEINFO;

typedef struct {
    int count;
    int cores[MAX_CORES];
} ResourceInfo;

#endif // types_h_INCLUDED
