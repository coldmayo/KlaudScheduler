This is code for a resource manager I am building for my upcoming Beowulf cluster.

I wanted to roll out some of my own software for my own learning purposes.

The plan:
- JSON file where queued and running jobs
- JSON file with node information
- A job dispatcher determines how/where the job is deployed
	- use sched_setaffinity() for a single CPU job
	- use ssh or OpenMPI to run tasks on multiple CPUs
