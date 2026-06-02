#include <stdio.h>

#include "scheduler_internal.h"

/* Compare two processes by arrival order. PID breaks exact arrival ties. */
int scheduler_arrival_precedes(const Process *candidate,
                               const Process *current)
{
    if (candidate->arrival_time != current->arrival_time) {
        return candidate->arrival_time < current->arrival_time;
    }

    return candidate->pid < current->pid;
}

/* Reset per-run state so each algorithm starts from the same workload. */
void scheduler_reset_states(ScheduleState states[],
                            const Process processes[],
                            int process_count)
{
    int i;

    for (i = 0; i < process_count; i++) {
        states[i].arrived = 0;
        states[i].completed = 0;
        states[i].remaining_cpu_time = processes[i].cpu_burst_time;
        states[i].executed_cpu_time = 0;
        states[i].io_remaining_time = 0;
        states[i].next_io_event = 0;
        states[i].ready_order = 0;
    }
}

/* Find the next process that has arrived but has not entered a ready queue. */
int scheduler_next_arrival(const Process processes[],
                           int process_count,
                           const ScheduleState states[],
                           int current_time)
{
    int i;
    int selected = -1;

    for (i = 0; i < process_count; i++) {
        if (states[i].arrived) {
            continue;
        }

        if (processes[i].arrival_time > current_time) {
            continue;
        }

        if (selected == -1) {
            selected = i;
            continue;
        }

        if (scheduler_arrival_precedes(&processes[i], &processes[selected])) {
            selected = i;
        }
    }

    return selected;
}

/* Check whether the running process has reached its next I/O trigger. */
int scheduler_io_due(const Process *process, const ScheduleState *state)
{
    int event_index;
    int trigger_time;

    if (state->next_io_event >= process->io_event_count) {
        return 0;
    }

    event_index = state->next_io_event;
    trigger_time = process->io_events[event_index].trigger_time;

    return state->executed_cpu_time == trigger_time;
}

/* Move a running process into the waiting queue for its next I/O event. */
void scheduler_start_io(WaitingQueue *waiting_queue,
                        const Process *process,
                        ScheduleState states[],
                        int process_index)
{
    int event_index;
    int io_duration;

    event_index = states[process_index].next_io_event;
    io_duration = process->io_events[event_index].duration;

    states[process_index].io_remaining_time = io_duration;
    states[process_index].next_io_event++;
    queue_enqueue(waiting_queue, process_index);
}

/* Record completion time and turnaround time for a finished process. */
void scheduler_finish_process(const Process *process,
                              ScheduleState *state,
                              int process_index,
                              int completion_time,
                              int *completed_count,
                              int *completion_times,
                              int *turnaround_times)
{
    state->completed = 1;
    completion_times[process_index] = completion_time;
    turnaround_times[process_index] = completion_time - process->arrival_time;
    (*completed_count)++;
}

/* Print per-process results and average metrics for one algorithm run. */
void scheduler_print_metrics(const char *algorithm_name,
                             const Process processes[],
                             int process_count,
                             const int completion_times[],
                             const int waiting_times[],
                             const int turnaround_times[])
{
    int i;
    int total_waiting_time = 0;
    int total_turnaround_time = 0;

    printf("\n%s Metrics\n", algorithm_name);
    printf("PID  Arrival  CPU Burst  I/O Total  Completion  Waiting  Turnaround\n");
    printf("---  -------  ---------  ---------  ----------  -------  ----------\n");

    for (i = 0; i < process_count; i++) {
        total_waiting_time += waiting_times[i];
        total_turnaround_time += turnaround_times[i];

        printf("P%-3d %-8d %-10d %-10d %-11d %-8d %-10d\n",
               processes[i].pid,
               processes[i].arrival_time,
               processes[i].cpu_burst_time,
               processes[i].io_burst_time,
               completion_times[i],
               waiting_times[i],
               turnaround_times[i]);
    }

    printf("\nAverage Waiting Time: %.2f\n",
           (double)total_waiting_time / process_count);
    printf("Average Turnaround Time: %.2f\n",
           (double)total_turnaround_time / process_count);
}
