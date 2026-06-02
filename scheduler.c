#include <stdio.h>

#include "gantt.h"
#include "process.h"
#include "queue.h"
#include "scheduler.h"

typedef struct {
    int arrived;
    int completed;
    int remaining_cpu_time;
    int executed_cpu_time;
    int io_remaining_time;
    int next_io_event;
} FcfsState;

/* FCFS tie-break: earlier arrival time first.
   If arrival times are equal, smaller PID goes first. */
static int fcfs_precedes(const Process *left, const Process *right)
{
    if (left->arrival_time != right->arrival_time) {
        return left->arrival_time < right->arrival_time;
    }

    return left->pid < right->pid;
}

static int next_fcfs(const Process processes[], int process_count, const int queued[])
{
    int i;
    int selected = -1;

    for (i = 0; i < process_count; i++) {
        if (queued[i]) {
            continue;
        }

        if (selected == -1) {
            selected = i;
            continue;
        }

        if (fcfs_precedes(&processes[i], &processes[selected])) {
            selected = i;
        }
    }

    return selected;
}

static void load_fcfs_order(ReadyQueue *ready_queue, const Process processes[], int process_count)
{
    int queued[MAX_PROCESSES] = {0};
    int i;
    int selected;

    queue_init(ready_queue);

    for (i = 0; i < process_count; i++) {
        int enqueued;

        selected = next_fcfs(processes, process_count, queued);
        if (selected == -1) {
            return;
        }

        enqueued = queue_enqueue(ready_queue, selected);
        if (enqueued == 0) {
            return;
        }

        queued[selected] = 1;
    }
}

static void reset_states(FcfsState states[],
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
    }
}

static int next_arrival(const Process processes[],
                        int process_count,
                        const FcfsState states[],
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

        if (fcfs_precedes(&processes[i], &processes[selected])) {
            selected = i;
        }
    }

    return selected;
}

static void admit_arrivals(ReadyQueue *ready_queue,
                           const Process processes[],
                           int process_count,
                           FcfsState states[],
                           int current_time)
{
    int process_index;

    /* Check arrivals for this time tick.
       Once a process enters the ready queue, mark it as arrived. */
    process_index = next_arrival(processes,
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

        process_index = next_arrival(processes,
                                     process_count,
                                     states,
                                     current_time);
    }
}

static void count_ready_wait(const ReadyQueue *ready_queue, int waiting_times[])
{
    int i;
    int position;

    /* Waiting time only counts time spent in the ready queue.
       Running processes and I/O waiting processes are not counted. */
    for (i = 0; i < ready_queue->size; i++) {
        position = (ready_queue->front + i) % QUEUE_CAPACITY;
        waiting_times[ready_queue->items[position]]++;
    }
}

static int io_due(const Process *process, const FcfsState *state)
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

static void start_io(WaitingQueue *waiting_queue,
                     const Process *process,
                     FcfsState states[],
                     int process_index)
{
    int event_index = states[process_index].next_io_event;
    int io_duration;

    /* The running process leaves the CPU for I/O. */
    io_duration = process->io_events[event_index].duration;

    states[process_index].io_remaining_time = io_duration;
    states[process_index].next_io_event++;
    queue_enqueue(waiting_queue, process_index);
}

static void finish_process(const Process *process,
                           FcfsState *state,
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

static void run_io(WaitingQueue *waiting_queue,
                   ReadyQueue *ready_queue,
                   const Process processes[],
                   FcfsState states[],
                   int waiting_count,
                   int current_time,
                   int *completed_count,
                   int completion_times[],
                   int turnaround_times[])
{
    int i;
    int process_index;

    /* Update only the processes that were already in waiting_queue
       when this tick started. */
    for (i = 0; i < waiting_count; i++) {
        int dequeued;

        /* Pop one process from waiting_queue so its I/O can progress. */
        dequeued = queue_dequeue(waiting_queue, &process_index);
        if (dequeued == 0) {
            return;
        }

        /* One time unit passes for this process's I/O work. */
        states[process_index].io_remaining_time--;

        /* If I/O is still not finished, keep it in waiting_queue. */
        if (states[process_index].io_remaining_time > 0) {
            queue_enqueue(waiting_queue, process_index);
            continue;
        }

        /* I/O is done. If no CPU work remains, the process is complete. */
        if (states[process_index].remaining_cpu_time == 0) {
            finish_process(&processes[process_index],
                           &states[process_index],
                           process_index,
                           current_time + 1,
                           completed_count,
                           completion_times,
                           turnaround_times);
        } else {
            /* Otherwise, send it back to ready_queue to wait for CPU. */
            queue_enqueue(ready_queue, process_index);
        }
    }
}

static void print_fcfs_metrics(const Process processes[],
                               int process_count,
                               const int completion_times[],
                               const int waiting_times[],
                               const int turnaround_times[])
{
    int i;
    int total_waiting_time = 0;
    int total_turnaround_time = 0;

    printf("\nFCFS Metrics\n");
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

void scheduler_run_fcfs(void)
{
    const Process *processes = process_get_all();
    int process_count = process_get_count();
    GanttChart gantt_chart;
    ReadyQueue initial_order;
    ReadyQueue ready_queue;
    WaitingQueue waiting_queue;
    FcfsState states[MAX_PROCESSES];
    int completion_times[MAX_PROCESSES] = {0};
    int waiting_times[MAX_PROCESSES] = {0};
    int turnaround_times[MAX_PROCESSES] = {0};
    int current_time = 0;
    int completed_count = 0;
    int running_process = -1;

    if (process_count == 0) {
        printf("\nNo processes have been created yet.\n");
        return;
    }

    load_fcfs_order(&initial_order, processes, process_count);
    queue_init(&ready_queue);
    queue_init(&waiting_queue);
    reset_states(states, processes, process_count);
    gantt_init(&gantt_chart);

    printf("\n[FCFS Scheduling]\n");
    printf("Tie-break rule: earlier arrival time, then smaller PID.\n");
    printf("I/O rule: each process uses its own trigger:duration events.\n\n");

    queue_print("Initial FCFS Order", &initial_order, processes);

    /* Discrete-time simulation.
       Each loop handles the interval [current_time, current_time + 1). */
    while (completed_count < process_count) {
        int waiting_count;
        int next_time;

        next_time = current_time + 1;
        admit_arrivals(&ready_queue,
                       processes,
                       process_count,
                       states,
                       current_time);

        if (running_process == -1) {
            int dequeued;

            dequeued = queue_dequeue(&ready_queue, &running_process);
            if (dequeued == 0) {
                running_process = -1;
            }
        }

        waiting_count = queue_size(&waiting_queue);

        if (running_process == -1) {
            gantt_add_segment(&gantt_chart, current_time, next_time, GANTT_IDLE_PID);
        } else {
            const Process *process = &processes[running_process];

            states[running_process].remaining_cpu_time--;
            states[running_process].executed_cpu_time++;
            gantt_add_segment(&gantt_chart, current_time, next_time, process->pid);
        }

        count_ready_wait(&ready_queue, waiting_times);

        if (running_process != -1) {
            const Process *process = &processes[running_process];
            int needs_io;
            int cpu_finished;

            needs_io = io_due(process, &states[running_process]);
            cpu_finished = states[running_process].remaining_cpu_time == 0;

            if (needs_io) {
                start_io(&waiting_queue, process, states, running_process);
                running_process = -1;
            } else {
                if (cpu_finished) {
                    finish_process(process,
                                   &states[running_process],
                                   running_process,
                                   next_time,
                                   &completed_count,
                                   completion_times,
                                   turnaround_times);
                    running_process = -1;
                }
            }
        }

        run_io(&waiting_queue,
               &ready_queue,
               processes,
               states,
               waiting_count,
               current_time,
               &completed_count,
               completion_times,
               turnaround_times);

        current_time++;
    }

    gantt_print_timeline(&gantt_chart);
    gantt_print(&gantt_chart);
    print_fcfs_metrics(processes,
                       process_count,
                       completion_times,
                       waiting_times,
                       turnaround_times);
}
