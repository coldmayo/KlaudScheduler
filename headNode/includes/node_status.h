#ifndef node_status_h_INCLUDED
#define node_status_h_INCLUDED

#include "types.h"

NODEINFO * node_info(const char * addr);
void updateNodeHealth(void);

#endif // node_status_h_INCLUDED
