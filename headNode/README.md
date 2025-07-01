This directory contains code that should be downloaded to the head node

Logistics:

- ```cJSON.c``` is a small json library I downloaded locally because it is easier, I did not write this code
- ```utils.c``` basically some smaller miscellaneous functions
- ```current_jobs.c``` this file deals with the status of and saving jobs to ```jobs.json```
- ```dispatch.c``` a file that deals with the process of allocating resources to jobs, and basically glues everything together
- ```main.c``` takes user command and puts jobs in the queue
- ```node_status.c``` monitors the core, total CPU, and RAM usage for each node
- ```nodes_list.c``` and ```nodes_list_main.c``` basically forms the ```nodes.json``` file which contains information about each node (# of cores, ip address, core availability, etc)
- ```node_status.txt``` is where CPU and RAM data is updated every 5 seconds
- ```ktrack.c``` shows information on jobs previously ran or in the queue

Take a look at the Makefile to see how it all fits together!
