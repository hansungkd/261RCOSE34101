#include <stdio.h>

#include "gantt.h"
#include "process.h"
#include "queue.h"
#include "scheduler.h"
#include "scheduler_internal.h"

/* Run Round Robin and save its metric result. */
static int run_rr(int time_quantum,
                  GanttChart *gantt_chart,
                  ScheduleResult *result)
{
    const Process *processes = process_get_all();
    int process_count = process_get_count();
    ReadyQueue ready_queue;
    WaitingQueue waiting_queue;
    ScheduleState states[MAX_PROCESSES];
    int completion_times[MAX_PROCESSES] = {0};
    int waiting_times[MAX_PROCESSES] = {0};
    int turnaround_times[MAX_PROCESSES] = {0};
    int response_times[MAX_PROCESSES];
    int current_time = 0;
    int completed_count = 0;
    int running_process = -1;
    int quantum_left = 0;

    if (process_count == 0) {
        return 0;
    }

    /* Prepare queues and metric arrays for this Round Robin run. */
    queue_init(&ready_queue);
    queue_init(&waiting_queue);
    scheduler_reset_states(states, processes, process_count);
    scheduler_init_responses(response_times);
    if (gantt_chart != 0) {
        gantt_init(gantt_chart);
    }

    while (completed_count < process_count) {
        int waiting_count;
        int next_time;

        next_time = current_time + 1;

        /* New arrivals join the rear of the FIFO ready queue. */
        scheduler_fifo_admit(&ready_queue,
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
            scheduler_add_gantt(gantt_chart,
                                current_time,
                                next_time,
                                GANTT_IDLE_PID);
        } else {
            const Process *process = &processes[running_process];

            scheduler_mark_response(process,
                                    running_process,
                                    current_time,
                                    response_times);
            states[running_process].remaining_cpu_time--;
            states[running_process].executed_cpu_time++;
            quantum_left--;
            scheduler_add_gantt(gantt_chart,
                                current_time,
                                next_time,
                                process->pid);
        }

        /* Ready Queue time is the waiting time measured for CPU scheduling. */
        scheduler_fifo_count_wait(&ready_queue, waiting_times);

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
        scheduler_fifo_run_io(&waiting_queue,
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

    scheduler_save_result(result,
                          processes,
                          process_count,
                          current_time,
                          completion_times,
                          waiting_times,
                          turnaround_times,
                          response_times);
    return 1;
}

/* Build only the Round Robin metric result for the comparison table. */
int scheduler_round_robin_result(ScheduleResult *result, int time_quantum)
{
    if (time_quantum < 1) {
        return 0;
    }

    return run_rr(time_quantum, 0, result);
}

/* Run Round Robin scheduling until every process is complete. */
void scheduler_run_round_robin(void)
{
    const Process *processes = process_get_all();
    int process_count = process_get_count();
    GanttChart gantt_chart;
    ScheduleResult result;
    int time_quantum;
    int ran;

    if (process_count == 0) {
        printf("\nNo processes have been created yet.\n");
        return;
    }

    time_quantum = scheduler_read_time_quantum();
    if (time_quantum < 1) {
        return;
    }

    printf("\n[Round Robin Scheduling]\n");
    printf("Time quantum: %d\n", time_quantum);
    printf("Rule: quantum expiration sends the process to the Ready Queue rear.\n");
    printf("I/O rule: I/O interrupts the current quantum and moves to Waiting Queue.\n\n");

    ran = run_rr(time_quantum, &gantt_chart, &result);
    if (ran == 0) {
        return;
    }

    /* Print the full result after all processes complete. */
    gantt_print_timeline(&gantt_chart);
    gantt_print(&gantt_chart);
    scheduler_print_metrics("Round Robin",
                            processes,
                            process_count,
                            &result);
    gantt_free(&gantt_chart);
}
