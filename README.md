# CPU Scheduling Simulator

Operating Systems term project/

## Build

```sh
make
```

## Run

```sh
./scheduler
```

## Current

- Interactive menu-based simulator shell
- Build system through `Makefile`
- `Process` structure with PID, arrival time, CPU burst, I/O burst, and priority
- Sample in-memory process table
- Menu option 1 creates a random process set
- Menu option 2 prints the current process set
- Menu option 3 runs a first FCFS scheduler using CPU burst time
- `queue.c` / `queue.h` provide Queue base structures
- `gantt.c` / `gantt.h` provide Gantt Chart output
- Process list accessor functions for future scheduler modules
- `scheduler.c` / `scheduler.h` provide scheduling algorithm implementations
- FCFS prints completion time, waiting time, turnaround time, and averages
- FCFS handles zero or more I/O events per process through the Waiting Queue
- FCFS waiting time counts only time spent in the Ready Queue

Current random ranges:

- Arrival time: 0-9
- CPU burst time: 1-10
- I/O event count: 0-3, limited by CPU burst time
- I/O trigger time: after 1 to `CPU burst - 1` CPU time units
- I/O duration per event: 1-5
- Priority: 1-5, where a smaller number means higher priority

## Plan

1. Add Non-Preemptive SJF.
2. Add result comparison for FCFS and Non-Preemptive SJF.
