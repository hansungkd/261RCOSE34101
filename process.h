#ifndef PROCESS_H
#define PROCESS_H

#define MAX_PROCESSES 10

typedef struct {
    int pid;
    int arrival_time;
    int cpu_burst_time;
    int io_burst_time;
    int priority;
} Process;

void process_print_all(void);

#endif
