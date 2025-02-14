This directory contains code that should be downloaded to the head node

Logistics:
- nodes.json holds all information about the nodes of the cluster. To populate this file, run the gen_nodes_list.sh file.
	- Basically, uses sockets to communicate over the network to get cpu + gpu data
	- Also saves the info to rankfile.txt just in case
- jobs.json holds all information about the submitted jobs, current_jobs for now is handling everything relating to that
- dispatch.c should be constantly running on the head node, it basically facilitates sending jobs to the correct nodes
