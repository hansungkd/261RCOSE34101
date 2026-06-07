#include <stdio.h>

#include "config.h"
#include "scheduler_internal.h"

/* Discard leftover input after a bad scheduler option value. */
static void clear_input_buffer(void)
{
    int ch;

    ch = getchar();
    while (ch != '\n') {
        if (ch == EOF) {
            return;
        }

        ch = getchar();
    }
}

/* Compare two processes by arrival order. PID breaks exact arrival ties. */
int scheduler_arrival_precedes(const Process *candidate,
                               const Process *current)
{
    if (candidate->arrival_time != current->arrival_time) {
        return candidate->arrival_time < current->arrival_time;
    }

    return candidate->pid < current->pid;
}

/* Read a positive Round Robin time quantum. */
int scheduler_read_time_quantum(void)
{
    int quantum;
    int result;

    printf("\nTime quantum (positive integer): ");
    result = scanf("%d", &quantum);

    if (result != 1) {
        printf("Invalid time quantum.\n");
        clear_input_buffer();
        return 0;
    }

    if (quantum < 1) {
        printf("Time quantum must be positive.\n");
        return 0;
    }

    return quantum;
}

/* Print the queue and scheduler configuration used by the simulator. */
void scheduler_print_config(void)
{
    printf("\n[System Configuration]\n");
    printf("CPU: single CPU simulator\n");
    printf("Context switch time: 0\n");
    printf("Waiting time: counted only while a process is in Ready Queue\n");
    printf("I/O model: Waiting Queue runs independently from CPU scheduling\n");
    printf("\nReady Queue\n");
    printf("- FCFS and Round Robin use FIFO queue order.\n");
    printf("- SJF uses a PriorityQueue ordered by shortest next CPU run.\n");
    printf("- Priority uses a PriorityQueue ordered by smaller priority number.\n");
    printf("\nWaiting Queue\n");
    printf("- A process moves here when its I/O trigger is reached.\n");
    printf("- After I/O finishes, the process returns to the matching Ready Queue.\n");

    config_print();
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

/* Add a Gantt segment only when the caller wants chart output. */
void scheduler_add_gantt(GanttChart *gantt_chart,
                         int start_time,
                         int end_time,
                         int pid)
{
    if (gantt_chart == 0) {
        return;
    }

    gantt_add_segment(gantt_chart, start_time, end_time, pid);
}

/* Add newly arrived processes to a FIFO ready queue. */
void scheduler_fifo_admit(ReadyQueue *ready_queue,
                          const Process processes[],
                          int process_count,
                          ScheduleState states[],
                          int current_time)
{
    int process_index;

    process_index = scheduler_next_arrival(processes,
                                           process_count,
                                           states,
                                           current_time);

    while (process_index != -1) {
        int enqueued;

        enqueued = queue_enqueue(ready_queue, process_index);
        if (enqueued == 0) {
            return;
        }

        states[process_index].arrived = 1;

        process_index = scheduler_next_arrival(processes,
                                               process_count,
                                               states,
                                               current_time);
    }
}

/* Count one ready-queue waiting tick for each process in a FIFO queue. */
void scheduler_fifo_count_wait(const ReadyQueue *ready_queue,
                               int waiting_times[])
{
    int i;
    int position;

    for (i = 0; i < ready_queue->size; i++) {
        position = (ready_queue->front + i) % QUEUE_CAPACITY;
        waiting_times[ready_queue->items[position]]++;
    }
}

/* Progress I/O and return finished I/O processes to a FIFO ready queue. */
void scheduler_fifo_run_io(WaitingQueue *waiting_queue,
                           ReadyQueue *ready_queue,
                           const Process processes[],
                           ScheduleState states[],
                           int waiting_count,
                           int current_time,
                           int *completed_count,
                           int completion_times[],
                           int turnaround_times[])
{
    int i;
    int process_index;

    for (i = 0; i < waiting_count; i++) {
        int dequeued;

        dequeued = queue_dequeue(waiting_queue, &process_index);
        if (dequeued == 0) {
            return;
        }

        states[process_index].io_remaining_time--;

        if (states[process_index].io_remaining_time > 0) {
            queue_enqueue(waiting_queue, process_index);
            continue;
        }

        if (states[process_index].remaining_cpu_time == 0) {
            scheduler_finish_process(&processes[process_index],
                                     &states[process_index],
                                     process_index,
                                     current_time + 1,
                                     completed_count,
                                     completion_times,
                                     turnaround_times);
        } else {
            queue_enqueue(ready_queue, process_index);
        }
    }
}

/* Add newly arrived processes to a PriorityQueue-based ready queue. */
void scheduler_pq_admit(PriorityQueue *ready_queue,
                        const Process processes[],
                        int process_count,
                        ScheduleState states[],
                        int current_time,
                        int *ready_order)
{
    int process_index;

    process_index = scheduler_next_arrival(processes,
                                           process_count,
                                           states,
                                           current_time);

    while (process_index != -1) {
        int pushed;

        states[process_index].ready_order = *ready_order;
        (*ready_order)++;

        pushed = pq_push(ready_queue, process_index);
        if (pushed == 0) {
            return;
        }

        states[process_index].arrived = 1;

        process_index = scheduler_next_arrival(processes,
                                               process_count,
                                               states,
                                               current_time);
    }
}

/* Count one ready-queue waiting tick for each process in a PriorityQueue. */
void scheduler_pq_count_wait(const PriorityQueue *ready_queue,
                             int waiting_times[])
{
    int i;

    for (i = 0; i < pq_size(ready_queue); i++) {
        int process_index;
        int found;

        found = pq_get(ready_queue, i, &process_index);
        if (found == 0) {
            return;
        }

        waiting_times[process_index]++;
    }
}

/* Progress I/O and return finished I/O processes to a PriorityQueue. */
void scheduler_pq_run_io(WaitingQueue *waiting_queue,
                         PriorityQueue *ready_queue,
                         const Process processes[],
                         ScheduleState states[],
                         int waiting_count,
                         int current_time,
                         int *ready_order,
                         int *completed_count,
                         int completion_times[],
                         int turnaround_times[])
{
    int i;
    int process_index;

    for (i = 0; i < waiting_count; i++) {
        int dequeued;

        dequeued = queue_dequeue(waiting_queue, &process_index);
        if (dequeued == 0) {
            return;
        }

        states[process_index].io_remaining_time--;

        if (states[process_index].io_remaining_time > 0) {
            queue_enqueue(waiting_queue, process_index);
            continue;
        }

        if (states[process_index].remaining_cpu_time == 0) {
            scheduler_finish_process(&processes[process_index],
                                     &states[process_index],
                                     process_index,
                                     current_time + 1,
                                     completed_count,
                                     completion_times,
                                     turnaround_times);
        } else {
            int pushed;

            states[process_index].ready_order = *ready_order;
            (*ready_order)++;

            pushed = pq_push(ready_queue, process_index);
            if (pushed == 0) {
                return;
            }
        }
    }
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

/* Fill response times with -1 so the first CPU run can be detected. */
void scheduler_init_responses(int response_times[])
{
    int i;

    for (i = 0; i < MAX_PROCESSES; i++) {
        response_times[i] = -1;
    }
}

/* Record how long a process waited before receiving CPU for the first time. */
void scheduler_mark_response(const Process *process,
                             int process_index,
                             int current_time,
                             int response_times[])
{
    if (response_times[process_index] != -1) {
        return;
    }

    response_times[process_index] = current_time - process->arrival_time;
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
                             const ScheduleResult *result)
{
    int i;

    printf("\n%s Metrics\n", algorithm_name);
    printf("PID  Arrival  CPU Burst  I/O Total  Completion  Waiting  Turnaround  Response\n");
    printf("---  -------  ---------  ---------  ----------  -------  ----------  --------\n");

    for (i = 0; i < process_count; i++) {
        printf("P%-3d %-8d %-10d %-10d %-11d %-8d %-11d %-8d\n",
               processes[i].pid,
               processes[i].arrival_time,
               processes[i].cpu_burst_time,
               processes[i].io_burst_time,
               result->completion_times[i],
               result->waiting_times[i],
               result->turnaround_times[i],
               result->response_times[i]);
    }

    printf("\nAverage Waiting Time: %.2f\n",
           result->average_waiting_time);
    printf("Average Turnaround Time: %.2f\n",
           result->average_turnaround_time);
    printf("Average Response Time: %.2f\n",
           result->average_response_time);
    printf("CPU Utilization: %.2f%%\n",
           result->cpu_utilization);
    printf("Throughput: %.4f processes/time unit\n",
           result->throughput);
}

/* Copy one simulation's metrics into a reusable result object. */
void scheduler_save_result(ScheduleResult *result,
                           const Process processes[],
                           int process_count,
                           int finish_time,
                           const int completion_times[],
                           const int waiting_times[],
                           const int turnaround_times[],
                           const int response_times[])
{
    int i;
    int total_waiting_time = 0;
    int total_turnaround_time = 0;
    int total_response_time = 0;
    int total_cpu_time = 0;

    if (result == 0) {
        return;
    }

    for (i = 0; i < MAX_PROCESSES; i++) {
        result->completion_times[i] = 0;
        result->waiting_times[i] = 0;
        result->turnaround_times[i] = 0;
        result->response_times[i] = 0;
    }

    for (i = 0; i < process_count; i++) {
        result->completion_times[i] = completion_times[i];
        result->waiting_times[i] = waiting_times[i];
        result->turnaround_times[i] = turnaround_times[i];
        result->response_times[i] = response_times[i];

        total_waiting_time += waiting_times[i];
        total_turnaround_time += turnaround_times[i];
        total_response_time += response_times[i];
        total_cpu_time += processes[i].cpu_burst_time;
    }

    result->finish_time = finish_time;
    result->average_waiting_time = 0.0;
    result->average_turnaround_time = 0.0;
    result->average_response_time = 0.0;
    result->cpu_utilization = 0.0;
    result->throughput = 0.0;

    if (process_count > 0) {
        result->average_waiting_time =
            (double)total_waiting_time / process_count;
        result->average_turnaround_time =
            (double)total_turnaround_time / process_count;
        result->average_response_time =
            (double)total_response_time / process_count;
    }

    if (finish_time > 0) {
        result->cpu_utilization = (double)total_cpu_time / finish_time * 100.0;
        result->throughput = (double)process_count / finish_time;
    }
}
