#include <stdio.h>

#include "gantt.h"
#include "priority_queue.h"
#include "process.h"
#include "queue.h"
#include "scheduler.h"
#include "scheduler_internal.h"

typedef struct {
    const Process *processes;
    const ScheduleState *states;
} PriorityContext;

/* Priority callback: smaller priority number has higher priority. */
static int has_priority(int candidate_index,
                        int current_index,
                        void *context)
{
    const PriorityContext *priority_context = context;
    const Process *candidate = &priority_context->processes[candidate_index];
    const Process *current = &priority_context->processes[current_index];
    const ScheduleState *states = priority_context->states;
    int candidate_order;
    int current_order;

    if (candidate->priority != current->priority) {
        return candidate->priority < current->priority;
    }

    candidate_order = states[candidate_index].ready_order;
    current_order = states[current_index].ready_order;
    if (candidate_order != current_order) {
        return candidate_order < current_order;
    }

    return candidate->pid < current->pid;
}

/* Add newly arrived processes to the priority ready queue. */
static void admit_arrivals(PriorityQueue *ready_queue,
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

/* Count waiting time for every process currently inside the priority queue. */
static void count_wait(const PriorityQueue *ready_queue, int waiting_times[])
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

/* Progress I/O and return finished I/O processes to the priority queue. */
static void run_io(WaitingQueue *waiting_queue,
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

/* Run non-preemptive Priority scheduling until every process is complete. */
void scheduler_run_nonpreemptive_priority(void)
{
    const Process *processes = process_get_all();
    int process_count = process_get_count();
    GanttChart gantt_chart;
    PriorityQueue ready_queue;
    WaitingQueue waiting_queue;
    ScheduleState states[MAX_PROCESSES];
    PriorityContext priority_context;
    int completion_times[MAX_PROCESSES] = {0};
    int waiting_times[MAX_PROCESSES] = {0};
    int turnaround_times[MAX_PROCESSES] = {0};
    int current_time = 0;
    int completed_count = 0;
    int running_process = -1;
    int ready_order = 0;

    if (process_count == 0) {
        printf("\nNo processes have been created yet.\n");
        return;
    }

    /* Give the priority queue access to process data and current state. */
    priority_context.processes = processes;
    priority_context.states = states;

    /* Prepare queues and metric arrays for this Priority run. */
    pq_init(&ready_queue, has_priority, &priority_context);
    queue_init(&waiting_queue);
    scheduler_reset_states(states, processes, process_count);
    gantt_init(&gantt_chart);

    printf("\n[Non-Preemptive Priority Scheduling]\n");
    printf("Selection rule: smaller priority number first.\n");
    printf("Tie-break rule: Ready Queue entry order, then smaller PID.\n");
    printf("I/O rule: each process uses its own trigger:duration events.\n\n");

    while (completed_count < process_count) {
        int waiting_count;
        int next_time;

        next_time = current_time + 1;

        /* New arrivals enter the priority ready queue at this time tick. */
        admit_arrivals(&ready_queue,
                       processes,
                       process_count,
                       states,
                       current_time,
                       &ready_order);

        /* Non-preemptive Priority selects only when the CPU is idle. */
        if (running_process == -1) {
            int selected;

            selected = pq_pop(&ready_queue, &running_process);
            if (selected == 0) {
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

        /* Every process still in the priority queue waited this tick. */
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

        /* I/O runs independently, then finished I/O returns to PriorityQueue. */
        run_io(&waiting_queue,
               &ready_queue,
               processes,
               states,
               waiting_count,
               current_time,
               &ready_order,
               &completed_count,
               completion_times,
               turnaround_times);

        current_time++;
    }

    /* Print the full result after all processes complete. */
    gantt_print_timeline(&gantt_chart);
    gantt_print(&gantt_chart);
    scheduler_print_metrics("Non-Preemptive Priority",
                            processes,
                            process_count,
                            completion_times,
                            waiting_times,
                            turnaround_times);
}
