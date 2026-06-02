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
} SjfContext;

/* Return the CPU time before the process finishes or reaches its next I/O. */
static int next_cpu_time(const Process *process, const ScheduleState *state)
{
    int next_io_index;
    int time_until_io;

    next_io_index = state->next_io_event;
    if (next_io_index >= process->io_event_count) {
        return state->remaining_cpu_time;
    }

    time_until_io = process->io_events[next_io_index].trigger_time
        - state->executed_cpu_time;

    if (time_until_io < state->remaining_cpu_time) {
        return time_until_io;
    }

    return state->remaining_cpu_time;
}

/* PriorityQueue callback for SJF: shorter next CPU run has priority. */
static int has_priority(int candidate_index,
                        int current_index,
                        void *context)
{
    const SjfContext *sjf_context = context;
    const Process *candidate = &sjf_context->processes[candidate_index];
    const Process *current = &sjf_context->processes[current_index];
    const ScheduleState *states = sjf_context->states;
    int candidate_cpu_time;
    int current_cpu_time;
    int candidate_order;
    int current_order;

    /* SJF policy: shorter next CPU run gets higher priority. */
    candidate_cpu_time = next_cpu_time(candidate, &states[candidate_index]);
    current_cpu_time = next_cpu_time(current, &states[current_index]);

    if (candidate_cpu_time != current_cpu_time) {
        return candidate_cpu_time < current_cpu_time;
    }

    candidate_order = states[candidate_index].ready_order;
    current_order = states[current_index].ready_order;
    if (candidate_order != current_order) {
        return candidate_order < current_order;
    }

    return candidate->pid < current->pid;
}

/* Return whether a ready process should preempt the running process. */
static int should_preempt(const Process processes[],
                          const ScheduleState states[],
                          int candidate_index,
                          int running_index)
{
    int candidate_cpu_time;
    int running_cpu_time;

    candidate_cpu_time = next_cpu_time(&processes[candidate_index],
                                       &states[candidate_index]);
    running_cpu_time = next_cpu_time(&processes[running_index],
                                     &states[running_index]);

    return candidate_cpu_time < running_cpu_time;
}

/* Pick a process every tick, but keep the current one unless a shorter job exists. */
static void choose_preemptive_process(PriorityQueue *ready_queue,
                                      const Process processes[],
                                      ScheduleState states[],
                                      int *running_process,
                                      int *ready_order)
{
    int candidate;
    int found;

    if (*running_process == -1) {
        int selected;

        selected = pq_pop(ready_queue, running_process);
        if (selected == 0) {
            *running_process = -1;
        }

        return;
    }

    found = pq_get(ready_queue, 0, &candidate);
    if (found == 0) {
        return;
    }

    if (should_preempt(processes, states, candidate, *running_process)) {
        int selected;

        states[*running_process].ready_order = *ready_order;
        (*ready_order)++;
        pq_push(ready_queue, *running_process);

        selected = pq_pop(ready_queue, running_process);
        if (selected == 0) {
            *running_process = -1;
        }
    }
}

/* Add newly arrived processes to the SJF priority queue. */
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

/* Progress I/O and return finished I/O processes to the SJF priority queue. */
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

/* Run non-preemptive SJF using a priority queue for ready-process selection. */
void scheduler_run_nonpreemptive_sjf(void)
{
    const Process *processes = process_get_all();
    int process_count = process_get_count();
    GanttChart gantt_chart;
    PriorityQueue ready_queue;
    WaitingQueue waiting_queue;
    ScheduleState states[MAX_PROCESSES];
    SjfContext sjf_context;
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
    sjf_context.processes = processes;
    sjf_context.states = states;

    /* Prepare queues and metric arrays for this SJF run. */
    pq_init(&ready_queue, has_priority, &sjf_context);
    queue_init(&waiting_queue);
    scheduler_reset_states(states, processes, process_count);
    gantt_init(&gantt_chart);

    printf("\n[Non-Preemptive SJF Scheduling]\n");
    printf("Selection rule: shortest next CPU run.\n");
    printf("Tie-break rule: Ready Queue entry order, then smaller PID.\n");
    printf("I/O rule: each process uses its own trigger:duration events.\n\n");

    while (completed_count < process_count) {
        int waiting_count;
        int next_time;

        next_time = current_time + 1;

        /* New arrivals enter the SJF priority queue at this time tick. */
        admit_arrivals(&ready_queue,
                       processes,
                       process_count,
                       states,
                       current_time,
                       &ready_order);

        /* Non-preemptive SJF selects only when the CPU is idle. */
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
    scheduler_print_metrics("Non-Preemptive SJF",
                            processes,
                            process_count,
                            completion_times,
                            waiting_times,
                            turnaround_times);
}

/* Run preemptive SJF by rechecking the shortest ready job every tick. */
void scheduler_run_preemptive_sjf(void)
{
    const Process *processes = process_get_all();
    int process_count = process_get_count();
    GanttChart gantt_chart;
    PriorityQueue ready_queue;
    WaitingQueue waiting_queue;
    ScheduleState states[MAX_PROCESSES];
    SjfContext sjf_context;
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
    sjf_context.processes = processes;
    sjf_context.states = states;

    /* Prepare queues and metric arrays for this SJF run. */
    pq_init(&ready_queue, has_priority, &sjf_context);
    queue_init(&waiting_queue);
    scheduler_reset_states(states, processes, process_count);
    gantt_init(&gantt_chart);

    printf("\n[Preemptive SJF Scheduling]\n");
    printf("Selection rule: shortest next CPU run at each time tick.\n");
    printf("Preemption rule: a ready process preempts only if it is shorter.\n");
    printf("Tie-break rule: keep the running process on equal length.\n");
    printf("I/O rule: each process uses its own trigger:duration events.\n\n");

    while (completed_count < process_count) {
        int waiting_count;
        int next_time;

        next_time = current_time + 1;

        /* New arrivals can preempt the current process on this tick. */
        admit_arrivals(&ready_queue,
                       processes,
                       process_count,
                       states,
                       current_time,
                       &ready_order);

        /* Preemptive SJF checks the best ready process every tick. */
        choose_preemptive_process(&ready_queue,
                                  processes,
                                  states,
                                  &running_process,
                                  &ready_order);

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
    scheduler_print_metrics("Preemptive SJF",
                            processes,
                            process_count,
                            completion_times,
                            waiting_times,
                            turnaround_times);
}
