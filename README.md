# Klaud Scheduler

This is code for a resource manager I am building for my Beowulf cluster. I wanted to roll out some of my own software for my own learning purposes after taking a Parallel Computing class.

## How it works

Obviously, the directories named 'headNode' and 'computeNode' contain code that should be downloaded on the head and compute nodes respectfully.

### Compute Node

Only one executable to worry about, ```gen_nodes_list``` is ran while the same executable on the head node is ran to communicate with the head node to give compute node info for nodes.json

### Head Node

There will be 4 executables made when running the Makefile.
1. ```gen-nodes```: Run this and this will populate the nodes.json file which gives information on all of the nodes and CPUs
	- If using TCP (specify in ```.klaudrc``` file):
		- First, run the ```gen_nodes_list.sh``` executable for the compute nodes FIRST, it will listen for a ping from the file on the head node
		- Make sure you change/add ip addressess (```nodes``` array) in the ```nodes_list_main.c``` file
			- Use REAL ip addressess and not aliases
	- If using SSH then just run the executable on the main node and you should be fine
2. ```dispatch```: Should be running all the time, will continuosly check if a new job is submitted. If it is it will put it in the queue and execute when ready (using mpirun)
3. ```klaudrun```: This is ran to submit a job into the queue
	- Arguments:
		- ```--num_cores```: Amount of cores you want to run the program with
		- ```--command```: Command used to actually run the executable
		- ```--output```: Outfile path
	- Example: ```main --num_cores=6 --command="./hello_mpi"```
4. ```ktrack```: Display previous/current jobs
	- Arguments:
		- ```--ID```: Specify the job id
		- ```--num_cores```: Specify the number of cores
		- ```--num_gpus```: Number of GPUs
		- ```--status```: Status (QUEUED, DONE, RUNNING)
	- Example: ```ktrack --num_cores=2```

Just a note, I have done minimal testing...

Check TODO.md to see what I will be doing next
