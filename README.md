# Klaud Scheduler

This is code for a lightweight resource manager I am building for my Beowulf cluster. I wanted to roll out some of my own software for my own learning purposes after taking a Parallel Computing class.

## Overveiw

The project is split into two main parts:
- `headNode/`: code that runs on the master/head node
	- Install this code for all accounts
- `computeNode/`: code that runs on worker/compute nodes
	- exists only for TCP communication to make nodes.json, if you use SSH to do this instead then feel free to delete

The scheduler is intended to:
- discover available worker nodes,
- track node CPU and memory usage,
- queue jobs,
- schedule jobs based on available resources,
- run jobs via MPI or shell commands,
- provide basic job monitoring through command-line tools

There will be 4 executables made when running the Makefile.
1. ```gen-nodes```: Run this and it will populate the nodes.json file which gives core information and availability to ```dispatch```
	- If using TCP (specify in ```.klaudrc``` file):
		- First, run the ```gen_nodes_list.sh``` executable for the compute nodes FIRST, it will listen for a ping from the file on the head node
		- Make sure you change/add ip addressess (```nodes``` array) in the ```nodes_list_main.c``` file
			- Use REAL ip addressess and not aliases
	- If using SSH then simply run the executable on the main node
2. ```dispatch```: Should be running all the time, will continuosly check if a new job is submitted. If it is it will put it in the queue and execute when ready (using mpirun)
3. ```klaudrun```: This is ran to submit a job into the queue
	- Arguments:
		- ```--num_cores```: Amount of cores you want to run the program with
		- ```--command```: Command used to actually run the executable
		- ```--output```: Outfile path
	- Example: ```klaudrun --num_cores=6 --command="./hello_mpi"```
	- If you want to instead use a .klaud file: ```klaudrun --file="pingus.klaud"```
4. ```ktrack```: Display previous/current jobs
	- Arguments:
		- ```--ID```: Specify the job id
		- ```--num_cores```: Specify the number of cores
		- ```--num_gpus```: Number of GPUs
		- ```--status```: Status (QUEUED, DONE, RUNNING)
	- Example: ```ktrack --num_cores=2```

## Repository layout

```text
.
├── README.md
├── TODO.md
├── headNode/
│   ├── Makefile
│   ├── README.md
│   ├── src/
│   ├── includes/
│   ├── examples/
│   ├── pingus.klaud
│   └── node_status.txt
└── computeNode/
    ├── README.md
    ├── send_data.c
    └── gen_nodes_list.sh
```

Check out <a href="https://mayawallach.xyz/articles/cluster1.html" target="_blank">this blog post</a> to see how to set up your own home cluster using KlaudScheduler.

Check TODO.md to see what I will be doing next
