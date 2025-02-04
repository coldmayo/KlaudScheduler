This is code for a resource manager I am building for my upcoming Beowulf cluster.

I wanted to roll out some of my own software for my own learning purposes.

How to:
We generate a file called nodes_list.json. This is a way to save all of the CPUs, GPUs, and their nodes. Run: gen_nodes_list.sh

The plan:
- JSON file where queued and running jobs
- JSON file with node information
- A job dispatcher determines how/where the job is deployed
	- use sched_setaffinity() for a single CPU job
	- use ssh or OpenMPI to run tasks on multiple CPUs
