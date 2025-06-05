Notes on the CPU/RAM health checker

I think we could probably read /proc/stat (CPU Usage) and /proc/meminfo (RAM Usage) of all of the nodes

Probably use sockets like before (when we generate the nodes lists) to get information from the other nodes

All files in /proc are updated every time they are accessed which is convient

# Structure of /proc/stat

Example:

```
name|user|nice|system|idle|iowait|irq|softirq
cpu  852714 3261 441599 14276441 41239 110301 51746 0 0 0   // total values for all of the other cpuN lines
cpu0 169153 622 83665 3586600 11142 68535 19891 0 0 0   // All of the cores
cpu1 242470 650 132082 3533282 9703 14901 12008 0 0 0
cpu2 241265 1223 119880 3549475 9775 13869 10311 0 0 0
cpu3 199825 764 105970 3607082 10617 12995 9535 0 0 0
```

I added the comments and the column names, these things do not show up in the actual /proc/stat

Here's a list of what all of the columns mean:
- user: normal processes executing in user mode
- nice: niced processes executing in user mode
- system: processes executing in kernel mode
- idle: twiddling thumbs
- iowait: waiting for I/O to complete
- irq: servicing interrupts
- softirq: servicing softirqs

# Structure of Meminfo

```
MemTotal:        7877132 kB
MemFree:          407468 kB
MemAvailable:    1710564 kB
Buffers:           80648 kB
Cached:          1916220 kB
SwapCached:            0 kB
Active:          2231048 kB
Inactive:        4051772 kB
Active(anon):    1776288 kB
Inactive(anon):  3192112 kB
Active(file):     454760 kB
Inactive(file):   859660 kB
Unevictable:      572592 kB
Mlocked:             116 kB
SwapTotal:             0 kB
SwapFree:              0 kB
Zswap:                 0 kB
Zswapped:              0 kB
Dirty:               692 kB
Writeback:             0 kB
AnonPages:       4710444 kB
Mapped:           791376 kB
Shmem:            682856 kB
KReclaimable:     296456 kB
Slab:             421416 kB
SReclaimable:     296456 kB
SUnreclaim:       124960 kB
KernelStack:       21296 kB
PageTables:        69548 kB
SecPageTables:      2056 kB
NFS_Unstable:          0 kB
Bounce:                0 kB
WritebackTmp:          0 kB
CommitLimit:     3938564 kB
Committed_AS:   15273824 kB
VmallocTotal:   34359738367 kB
VmallocUsed:       57836 kB
VmallocChunk:          0 kB
Percpu:             2960 kB
HardwareCorrupted:     0 kB
AnonHugePages:   1267712 kB
ShmemHugePages:        0 kB
ShmemPmdMapped:        0 kB
FileHugePages:    249856 kB
FilePmdMapped:    247808 kB
CmaTotal:              0 kB
CmaFree:               0 kB
Unaccepted:            0 kB
HugePages_Total:       0
HugePages_Free:        0
HugePages_Rsvd:        0
HugePages_Surp:        0
Hugepagesize:       2048 kB
Hugetlb:               0 kB
DirectMap4k:      153388 kB
DirectMap2M:     4833280 kB
DirectMap1G:     3145728 kB
```

Keep in mind: RAM used = MemTotal − (MemFree + Buffers + Cached + SReclaimable)

# Implementation ideas

- Straight up using ssh:
```code
ssh $node "cat /proc/meminfo | grep Mem"
```
- Maybe write an OpenMPI script for this? Might be quicker as we expand the # of nodes
