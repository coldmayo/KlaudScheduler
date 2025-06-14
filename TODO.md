Open up an issue for any features you want to see in the future!

- [ ] Make better Documentation, it sucks at the moment
- [x] Make more MPI examples
	- Made a MPI program that calculates the digits of pi using the Monte Carlo method
- [ ] Figure out how this would work in a situation with muliple users
- [x] Make executing mpirun and the scheduler work on different threads (things are kind of running linearly at the moment)
	- The goal is to be able to run multiple MPI programs in parallel
	- [ ] I put execute_job() on a different thread, TEST THIS
- [x] Add some way to monitor CPU health
	- [ ] I added node_status.c to track the usage of cpus/cores and ram usage, TEST THIS
- [ ] Better error handling with user input
	- What if a user enters the wrong command?
	- What if files are not able to be obtained?
	- etc
- [x] Take actual time into account instead of counting the # of jobs ran before
	- TEST THIS
- [ ] Similarly to .slurm files, make a way to submit a job through a .klaud file instead
- [ ] GPU stuff
	- Don't have any GPUs in my cluster atm so I can't develop for or test this yet
