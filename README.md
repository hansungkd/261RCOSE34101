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
- Process list accessor functions for future scheduler modules
- `scheduler.c` / `scheduler.h` provide scheduling algorithm implementations

Current random ranges:

- Arrival time: 0-9
- CPU burst time: 1-10
- I/O burst time: 0-5
- Priority: 1-5, where a smaller number means higher priority

## Plan

1. Add Gantt Chart and waiting/turnaround metrics to FCFS.
2. Add I/O handling through the Waiting Queue.
