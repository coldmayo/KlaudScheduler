#ifndef node_status_h_INCLUDED
#define node_status_h_INCLUDED

typedef struct {
	float cpuUse;
	int num_cores;
	float coreUse[12];
	float ram_usage;
} NODEINFO;

NODEINFO * node_info(char * hostname);

#endif // node_status_h_INCLUDED
