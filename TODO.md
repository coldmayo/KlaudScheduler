Open up an issue for any features you want to see in the future!

# Done

- [x] Make better Documentation, it sucks at the moment
	- I guess I will be working on this as I go along
- [x] Make more MPI examples
	- Made a MPI program that calculates the digits of pi using the Monte Carlo method
- [x] Make executing mpirun and the scheduler work on different threads (things are kind of running linearly at the moment)
	- [x] I put execute_job() on a different thread, TEST THIS
- [x] Add some way to monitor CPU health
	- [x] I added node_status.c to track the usage of cpus/cores and ram usage, TEST THIS
- [x] Take actual time into account instead of counting the # of jobs ran before
	- [x] TEST THIS
- [x] Similarly to .slurm files, make a way to submit a job through a .klaud file instead
	- Mostly works, test to see if the command actually works
- [x] Figure out how to make ./main an actual global command, seems like it would be convienent
- [x] Integrate the use of the klaudconfig file
	- [x] Use it to configure proirity function
	- [x] Can be used to exclude aliases
		- Come up with more uses for this
- [x] Instead of the current nodes.json population strat, use ssh to get node info (like node_status.c)
	- [x] TEST this out
	- [x] Also use klaudconfig to filter out nodes not needed :>

# Not Done

- [ ] Figure out how this would work in a situation with muliple users
	- [ ] When this does happen, implement Fair Share for priority system
- [ ] Add a way for the user to set a maximum runtime
- [ ] Add a working directory field for jobs.json
	- You can run all of the executables from anywhere (saved to HOME/.local/bin/)
- [ ] Add a command where the user can check the resources used in the cluster? Similar to sinfo in slurm
- [ ] For the ktrack command add a way to add comma delimited args into 1 filter
	- i.e. --ID=1000,1001

# Can't do rn :(

- [ ] GPU stuff
	- Don't have any GPUs in my cluster atm so I can't develop for or test this yet
	- Find a way to expand the nodes list to GPUs as well
- [ ] Email notifications to alert when a job starts running and finishes
	- IP Forwarding is still an issue, so we will see when this can get done

