#ifndef SCHEDULER_INTERNAL_H
#define SCHEDULER_INTERNAL_H

#include "process.h"
#include "queue.h"

typedef struct {
    int arrived;
    int completed;
    int remaining_cpu_time;
    int executed_cpu_time;
    int io_remaining_time;
    int next_io_event;
    int ready_order;
} ScheduleState;

int scheduler_arrival_precedes(const Process *candidate,
                               const Process *current);
void scheduler_reset_states(ScheduleState states[],
                            const Process processes[],
                            int process_count);
int scheduler_next_arrival(const Process processes[],
                           int process_count,
                           const ScheduleState states[],
                           int current_time);
int scheduler_io_due(const Process *process, const ScheduleState *state);
void scheduler_start_io(WaitingQueue *waiting_queue,
                        const Process *process,
                        ScheduleState states[],
                        int process_index);
void scheduler_finish_process(const Process *process,
                              ScheduleState *state,
                              int process_index,
                              int completion_time,
                              int *completed_count,
                              int *completion_times,
                              int *turnaround_times);
void scheduler_print_metrics(const char *algorithm_name,
                             const Process processes[],
                             int process_count,
                             const int completion_times[],
                             const int waiting_times[],
                             const int turnaround_times[]);

#endif
