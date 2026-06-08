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
- Top-level menu follows Create_Process, Config, Schedule, Evaluation
- Create_Process submenu creates and prints process sets
- Create_Process submenu can load a fixed process set from an input file
- Config submenu prints and updates runtime simulator settings
- Schedule submenu runs FCFS, SJF, Priority, and Round Robin
- Evaluation submenu compares scheduling results
- `config.c` / `config.h` provide runtime configuration values
- `queue.c` / `queue.h` provide FIFO Queue base structures
- `priority_queue.c` / `priority_queue.h` provide heap-based Priority Queue selection
- `gantt.c` / `gantt.h` provide dynamically growing Gantt Chart output
- Process list accessor functions for future scheduler modules
- `scheduler.c` / `scheduler_internal.h` provide shared scheduling helpers
- `scheduler.c` prints queue/system configuration
- `scheduler_fcfs.c` provides FCFS scheduling
- `scheduler_sjf.c` provides Non-Preemptive and Preemptive SJF scheduling
- `scheduler_priority.c` provides Non-Preemptive and Preemptive Priority scheduling
- `scheduler_rr.c` provides Round Robin scheduling
- `scheduler_compare.c` provides average metric comparison output
- `scheduler.h` provides public scheduler function declarations
- FCFS prints completion time, waiting time, turnaround time, and averages
- FCFS handles zero or more I/O events per process through the Waiting Queue
- FCFS waiting time counts only time spent in the Ready Queue
- Non-Preemptive SJF uses the same I/O and metric model as FCFS
- Non-Preemptive SJF uses Priority Queue push/pop for Ready Queue selection
- Preemptive SJF checks for shorter ready jobs at every time tick
- Non-Preemptive Priority uses smaller priority number as higher priority
- Preemptive Priority checks for higher-priority ready jobs at every time tick
- Round Robin asks for a time quantum and requeues unfinished processes when it expires
- Result comparison runs all implemented algorithms on the same current process set
- Additional metrics: average response time, CPU utilization, and throughput
- Long Gantt output is split into readable pages with Enter prompts
- Create random process asks for a seed; 0 uses a time-based seed

Default runtime random ranges:

- Process limit: 10, bounded by `MAX_PROCESSES`
- Arrival time: 0-9
- CPU burst time: 1-10
- I/O event count: 0-3, bounded by `MAX_IO_EVENTS` and CPU burst time
- I/O trigger time: after 1 to `CPU burst - 1` CPU time units
- I/O duration per event: 1-5
- Priority: 1-5, where a smaller number means higher priority

## Fixed Input File Format

Each non-empty line describes one process:

```text
pid arrival_time cpu_burst_time priority io_event_count [io_trigger io_duration]...
```

Rules:

- `pid` must be unique and positive
- `arrival_time` must be 0 or greater
- `cpu_burst_time` and `priority` must be positive
- `io_event_count` must be between 0 and `MAX_IO_EVENTS`
- each I/O trigger must be after at least 1 CPU time unit and before the process finishes
- blank lines and text after `#` are ignored

Example:

```text
# pid arrival cpu priority io_count [trigger duration]...
1 0 8 2 1 4 3
2 1 4 1 1 2 2
3 2 9 3 0
```

Sample files are in the `sample/` directory:

- `sample/basic_io.txt`
- `sample/no_io.txt`
- `sample/io_idle.txt`
- `sample/same_arrival.txt`
- `sample/priority_focus.txt`
- `sample/round_robin_focus.txt`
