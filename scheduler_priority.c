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

/* Return whether a ready process should preempt the running process. */
static int should_preempt(const Process processes[],
                          int candidate_index,
                          int running_index)
{
    return processes[candidate_index].priority < processes[running_index].priority;
}

/* Pick a process every tick, but keep the current one unless priority is higher. */
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

    if (should_preempt(processes, candidate, *running_process)) {
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

/* Run one Priority variant and save its metric result. */
static int run_priority(int preemptive,
                        GanttChart *gantt_chart,
                        ScheduleResult *result)
{
    const Process *processes = process_get_all();
    int process_count = process_get_count();
    PriorityQueue ready_queue;
    WaitingQueue waiting_queue;
    ScheduleState states[MAX_PROCESSES];
    PriorityContext priority_context;
    int completion_times[MAX_PROCESSES] = {0};
    int waiting_times[MAX_PROCESSES] = {0};
    int turnaround_times[MAX_PROCESSES] = {0};
    int response_times[MAX_PROCESSES];
    int current_time = 0;
    int completed_count = 0;
    int running_process = -1;
    int ready_order = 0;

    if (process_count == 0) {
        return 0;
    }

    /* Give the priority queue access to process data and current state. */
    priority_context.processes = processes;
    priority_context.states = states;

    /* Prepare queues and metric arrays for this Priority run. */
    pq_init(&ready_queue, has_priority, &priority_context);
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

        /* New arrivals enter the priority ready queue at this time tick. */
        scheduler_pq_admit(&ready_queue,
                           processes,
                           process_count,
                           states,
                           current_time,
                           &ready_order);

        if (preemptive) {
            /* Preemptive Priority checks the best ready process every tick. */
            choose_preemptive_process(&ready_queue,
                                      processes,
                                      states,
                                      &running_process,
                                      &ready_order);
        } else {
            /* Non-preemptive Priority selects only when the CPU is idle. */
            if (running_process == -1) {
                int selected;

                selected = pq_pop(&ready_queue, &running_process);
                if (selected == 0) {
                    running_process = -1;
                }
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
            scheduler_add_gantt(gantt_chart,
                                current_time,
                                next_time,
                                process->pid);
        }

        /* Every process still in the priority queue waited this tick. */
        scheduler_pq_count_wait(&ready_queue, waiting_times);

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
        scheduler_pq_run_io(&waiting_queue,
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

/* Build only the Non-Preemptive Priority metric result for comparison. */
int scheduler_nonpreemptive_priority_result(ScheduleResult *result)
{
    return run_priority(0, 0, result);
}

/* Build only the Preemptive Priority metric result for comparison. */
int scheduler_preemptive_priority_result(ScheduleResult *result)
{
    return run_priority(1, 0, result);
}

/* Run non-preemptive Priority scheduling until every process is complete. */
void scheduler_run_nonpreemptive_priority(void)
{
    const Process *processes = process_get_all();
    int process_count = process_get_count();
    GanttChart gantt_chart;
    ScheduleResult result;
    int ran;

    if (process_count == 0) {
        printf("\nNo processes have been created yet.\n");
        return;
    }

    printf("\n[Non-Preemptive Priority Scheduling]\n");
    printf("Selection rule: smaller priority number first.\n");
    printf("Tie-break rule: Ready Queue entry order, then smaller PID.\n");
    printf("I/O rule: each process uses its own trigger:duration events.\n\n");

    ran = run_priority(0, &gantt_chart, &result);
    if (ran == 0) {
        return;
    }

    /* Print the full result after all processes complete. */
    gantt_print_timeline(&gantt_chart);
    gantt_print(&gantt_chart);
    scheduler_print_metrics("Non-Preemptive Priority",
                            processes,
                            process_count,
                            &result);
    gantt_free(&gantt_chart);
}

/* Run preemptive Priority scheduling until every process is complete. */
void scheduler_run_preemptive_priority(void)
{
    const Process *processes = process_get_all();
    int process_count = process_get_count();
    GanttChart gantt_chart;
    ScheduleResult result;
    int ran;

    if (process_count == 0) {
        printf("\nNo processes have been created yet.\n");
        return;
    }

    printf("\n[Preemptive Priority Scheduling]\n");
    printf("Selection rule: smaller priority number first at each time tick.\n");
    printf("Preemption rule: a ready process preempts only if priority is higher.\n");
    printf("Tie-break rule: keep the running process on equal priority.\n");
    printf("I/O rule: each process uses its own trigger:duration events.\n\n");

    ran = run_priority(1, &gantt_chart, &result);
    if (ran == 0) {
        return;
    }

    /* Print the full result after all processes complete. */
    gantt_print_timeline(&gantt_chart);
    gantt_print(&gantt_chart);
    scheduler_print_metrics("Preemptive Priority",
                            processes,
                            process_count,
                            &result);
    gantt_free(&gantt_chart);
}
