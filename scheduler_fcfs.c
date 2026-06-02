#include <stdio.h>

#include "gantt.h"
#include "process.h"
#include "queue.h"
#include "scheduler.h"
#include "scheduler_internal.h"

/* Pick the not-yet-queued process that should appear next in FCFS order. */
static int next_order(const Process processes[],
                      int process_count,
                      const int queued[])
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

        if (scheduler_arrival_precedes(&processes[i], &processes[selected])) {
            selected = i;
        }
    }

    return selected;
}

/* Build the initial FCFS order for display. Actual admission still uses time. */
static void load_order(ReadyQueue *ready_queue,
                       const Process processes[],
                       int process_count)
{
    int queued[MAX_PROCESSES] = {0};
    int i;
    int selected;

    queue_init(ready_queue);

    for (i = 0; i < process_count; i++) {
        int enqueued;

        selected = next_order(processes, process_count, queued);
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

/* Add newly arrived processes to the FIFO ready queue. */
static void admit_arrivals(ReadyQueue *ready_queue,
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

/* Add one waiting-time tick to every process still in the FIFO ready queue. */
static void count_wait(const ReadyQueue *ready_queue, int waiting_times[])
{
    int i;
    int position;

    /* Waiting time is counted only while a process stays in Ready Queue. */
    for (i = 0; i < ready_queue->size; i++) {
        position = (ready_queue->front + i) % QUEUE_CAPACITY;
        waiting_times[ready_queue->items[position]]++;
    }
}

/* Progress I/O and return finished I/O processes to the FCFS ready queue. */
static void run_io(WaitingQueue *waiting_queue,
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

/* Run the FCFS simulation until every process is complete. */
void scheduler_run_fcfs(void)
{
    const Process *processes = process_get_all();
    int process_count = process_get_count();
    GanttChart gantt_chart;
    ReadyQueue initial_order;
    ReadyQueue ready_queue;
    WaitingQueue waiting_queue;
    ScheduleState states[MAX_PROCESSES];
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

    /* Prepare queues and metric arrays for this FCFS run. */
    load_order(&initial_order, processes, process_count);
    queue_init(&ready_queue);
    queue_init(&waiting_queue);
    scheduler_reset_states(states, processes, process_count);
    gantt_init(&gantt_chart);

    printf("\n[FCFS Scheduling]\n");
    printf("Tie-break rule: earlier arrival time, then smaller PID.\n");
    printf("I/O rule: each process uses its own trigger:duration events.\n\n");

    queue_print("Initial FCFS Order", &initial_order, processes);

    while (completed_count < process_count) {
        int waiting_count;
        int next_time;

        next_time = current_time + 1;

        /* New arrivals enter the FIFO ready queue at this time tick. */
        admit_arrivals(&ready_queue,
                       processes,
                       process_count,
                       states,
                       current_time);

        /* FCFS only selects a new process when the CPU is idle. */
        if (running_process == -1) {
            int dequeued;

            dequeued = queue_dequeue(&ready_queue, &running_process);
            if (dequeued == 0) {
                running_process = -1;
            }
        }

        /* Only processes already waiting now should receive I/O progress. */
        waiting_count = queue_size(&waiting_queue);

        /* Run the CPU for one tick, or record IDLE if nobody is ready. */
        if (running_process == -1) {
            gantt_add_segment(&gantt_chart,
                              current_time,
                              next_time,
                              GANTT_IDLE_PID);
        } else {
            const Process *process = &processes[running_process];

            states[running_process].remaining_cpu_time--;
            states[running_process].executed_cpu_time++;
            gantt_add_segment(&gantt_chart,
                              current_time,
                              next_time,
                              process->pid);
        }

        /* Ready Queue time is the waiting time measured for CPU scheduling. */
        count_wait(&ready_queue, waiting_times);

        /* After one CPU tick, the running process may request I/O or finish. */
        if (running_process != -1) {
            const Process *process = &processes[running_process];
            int needs_io;
            int cpu_finished;

            needs_io = scheduler_io_due(process, &states[running_process]);
            cpu_finished = states[running_process].remaining_cpu_time == 0;

            if (needs_io) {
                scheduler_start_io(&waiting_queue,
                                   process,
                                   states,
                                   running_process);
                running_process = -1;
            } else {
                if (cpu_finished) {
                    scheduler_finish_process(process,
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

        /* I/O runs independently, then finished I/O returns to Ready Queue. */
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

    /* Print the full result after all processes complete. */
    gantt_print_timeline(&gantt_chart);
    gantt_print(&gantt_chart);
    scheduler_print_metrics("FCFS",
                            processes,
                            process_count,
                            completion_times,
                            waiting_times,
                            turnaround_times);
}
