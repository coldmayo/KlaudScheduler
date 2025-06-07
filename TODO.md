Open up an issue for any features you want to see in the future!

- [ ] Make better Documentation, it sucks at the moment
- [ ] Make more MPI examples
- [ ] Figure out how this would work in a situation with muliple users
- [x] Make executing mpirun and the scheduler work on different threads (things are kind of running linearly at the moment)
	- The goal is to be able to run multiple MPI programs in parallel
	- [ ] I put execute_job() on a different thread, TEST THIS
- [x] Add some way to monitor CPU health
	- [ ] I added node_status.c to track the usage of cpus/cores and ram usage, TEST THIS
- [ ] GPU stuff
