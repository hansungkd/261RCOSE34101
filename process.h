#ifndef PROCESS_H
#define PROCESS_H

#define MAX_PROCESSES 10
#define MAX_IO_EVENTS 3

typedef struct {
    int trigger_time;
    int duration;
} IoEvent;

typedef struct {
    int pid;
    int arrival_time;
    int cpu_burst_time;
    int io_burst_time;
    int priority;
    int io_event_count;
    IoEvent io_events[MAX_IO_EVENTS];
} Process;

void process_create_random(void);
void process_seed_random(int seed);
void process_generate_random(int count);
int process_load_from_file(const char *path);
void process_print_all(void);
int process_get_count(void);
const Process *process_get_all(void);
const Process *process_get_at(int index);
void process_restore_set(const Process source[], int count);

#endif
