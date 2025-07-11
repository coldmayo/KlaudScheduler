# Setting up

This is a guide if you want to help me test the Klaud Resource Manager.

## What you'll need

Hardware wise:
- 2 or more computers connected via network switch
	- 1 head (or master) node
	- 1 or more worker (or compute) node
- Possibly a USB to move files over
	- My ISP does not allow IP forwarding, check if yours does too
- Whatever you need to operate a computer: keyboard, mouse, etc
- A huge love for Klaud, the greatest Star Wars character of all time

Software wise:
- The KlaudScheduler of course
- Some stuff for the C programming language: nfs-utils, gcc, gcc-c++, openmpi, openmpi-devel, gdb, make, etc
- Set up static ips
- Set up passwordless ssh
- Set up NFS

## Set up /etc/hosts

Make sure to update the hosts file for each computer. It is important that every static ip has an alias such as below:
```
# /etc/hosts Example
192.168.1.100 master
192.168.1.101 node1
192.168.1.102 node2
```

## Compile everything
On the master node simply run the makefile (in the ```head node/``` directory):

```
$ make
```

## Make .klaudrc file
Make the config file and put it into your home folder i.e. ```~/.klaudrc``` of this form:

```
# Set priority system
# Options: First In First Out (FIFO), Fair Share (FS) [No way to do this now, do later]
# Shortest Job First (SJF), Best Fit (BF)
# At the moment we only support FIFO and SJF

set-priority FIFO
set-priority-lottery false
set-priority-aging true

# Set the ips to ignore (ip addresses not aliases)
# ignore-host 192.168.100.112

# For populating nodes.json, specify if you would prefer to use TCP or SHH
set-nodes-strat SSH
```

## Fill in nodes.json

If using TCP, On the compute nodes run ```gen_nodes_list.sh``` and then on the master node run ```get-nodes```
If using SSH, just run the ```get-nodes``` executables on the head node
You should now see the ```nodes.json``` has at least 1 json entry

## Start up the scheduler

- Run ```dispatch``` to start the job distributer
- Run ```klaudrun --num_cores==<num> --command="cmd"``` to submit some jobs to ```jobs.json```
