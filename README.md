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
- Menu option 2 prints the current process set

## Plan

1. Add random process generation.
2. Add Ready Queue and Waiting Queue structures.
3. Implement FCFS.
4. Add Gantt Chart and waiting/turnaround metrics.
