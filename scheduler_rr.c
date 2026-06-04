#include <stdio.h>

#include "gantt.h"
#include "process.h"
#include "queue.h"
#include "scheduler.h"
#include "scheduler_internal.h"

/* Discard leftover input after a bad time quantum value. */
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

/* Read the Round Robin time quantum from the user. */
static int read_time_quantum(void)
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

/* Add newly arrived processes to the Round Robin FIFO ready queue. */
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

/* Add one waiting-time tick to every process still in the ready queue. */
static void count_wait(const ReadyQueue *ready_queue, int waiting_times[])
{
    int i;
    int position;

    for (i = 0; i < ready_queue->size; i++) {
        position = (ready_queue->front + i) % QUEUE_CAPACITY;
        waiting_times[ready_queue->items[position]]++;
    }
}

/* Progress I/O and return finished I/O processes to the FIFO ready queue. */
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

/* Run Round Robin scheduling until every process is complete. */
void scheduler_run_round_robin(void)
{
    const Process *processes = process_get_all();
    int process_count = process_get_count();
    GanttChart gantt_chart;
    ReadyQueue ready_queue;
    WaitingQueue waiting_queue;
    ScheduleState states[MAX_PROCESSES];
    int completion_times[MAX_PROCESSES] = {0};
    int waiting_times[MAX_PROCESSES] = {0};
    int turnaround_times[MAX_PROCESSES] = {0};
    int current_time = 0;
    int completed_count = 0;
    int running_process = -1;
    int time_quantum;
    int quantum_left = 0;

    if (process_count == 0) {
        printf("\nNo processes have been created yet.\n");
        return;
    }

    time_quantum = read_time_quantum();
    if (time_quantum == 0) {
        return;
    }

    /* Prepare queues and metric arrays for this Round Robin run. */
    queue_init(&ready_queue);
    queue_init(&waiting_queue);
    scheduler_reset_states(states, processes, process_count);
    gantt_init(&gantt_chart);

    printf("\n[Round Robin Scheduling]\n");
    printf("Time quantum: %d\n", time_quantum);
    printf("Rule: quantum expiration sends the process to the Ready Queue rear.\n");
    printf("I/O rule: I/O interrupts the current quantum and moves to Waiting Queue.\n\n");

    while (completed_count < process_count) {
        int waiting_count;
        int next_time;

        next_time = current_time + 1;

        /* New arrivals join the rear of the FIFO ready queue. */
        admit_arrivals(&ready_queue,
                       processes,
                       process_count,
                       states,
                       current_time);

        /* When the CPU is free, take the oldest ready process. */
        if (running_process == -1) {
            int dequeued;

            dequeued = queue_dequeue(&ready_queue, &running_process);
            if (dequeued == 0) {
                running_process = -1;
            } else {
                quantum_left = time_quantum;
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
            quantum_left--;
            gantt_add_segment(&gantt_chart,
                              current_time,
                              next_time,
                              process->pid);
        }

        /* Ready Queue time is the waiting time measured for CPU scheduling. */
        count_wait(&ready_queue, waiting_times);

        /* After one CPU tick, handle I/O, completion, or quantum expiration. */
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
                quantum_left = 0;
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
                    quantum_left = 0;
                } else {
                    if (quantum_left == 0) {
                        queue_enqueue(&ready_queue, running_process);
                        running_process = -1;
                    }
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
    scheduler_print_metrics("Round Robin",
                            processes,
                            process_count,
                            completion_times,
                            waiting_times,
                            turnaround_times);
}
