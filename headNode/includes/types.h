#ifndef types_h_INCLUDED
#define types_h_INCLUDED

#include "../src/cJSON.h"
#include <time.h>
#include <stdbool.h>
#define MAX_CORES 256
#define MAX_NODES 100

typedef struct {
    int num_nodes;
    int core_nums[MAX_CORES];
} CLUSTERINFO;

typedef struct {
	int num_nodes;
	int cores[MAX_NODES][MAX_CORES];
	int cores_in_use[MAX_NODES][MAX_CORES];
	int tot_core[MAX_NODES];
	int RAM[MAX_NODES];
} HEALTH;

typedef struct {
	int * cpu_cores;
	char * hosts[100];
	int num_hosts;
	bool free;
} CPUout;

typedef struct {
	cJSON * job;
	CPUout * c;
	time_t start;
} EXECUTE;

typedef struct {
    int cores;
    char ** commands;
    int num_commands;
    char *command;
    char * outfile;
    time_t max_time;
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

typedef struct {
	char * priority_type;
	bool lottery;
	bool aging;
	char ** ignore_hosts;
	char * get_nodes_strat;
	char * dir;
	char * data_dir;
} ConfigInfo;

#endif // types_h_INCLUDED
