#include <stdio.h>

#include "gantt.h"
#include "process.h"
#include "queue.h"
#include "scheduler.h"

static int fcfs_precedes(const Process *left, const Process *right)
{
    if (left->arrival_time != right->arrival_time) {
        return left->arrival_time < right->arrival_time;
    }

    return left->pid < right->pid;
}

static int find_next_fcfs_process(const Process processes[], int process_count, const int queued[])
{
    int i;
    int selected = -1;

    for (i = 0; i < process_count; i++) {
        if (queued[i]) {
            continue;
        }

        if (selected == -1 || fcfs_precedes(&processes[i], &processes[selected])) {
            selected = i;
        }
    }

    return selected;
}

static void build_fcfs_queue(ReadyQueue *ready_queue, const Process processes[], int process_count)
{
    int queued[MAX_PROCESSES] = {0};
    int i;
    int selected;

    queue_init(ready_queue);

    for (i = 0; i < process_count; i++) {
        selected = find_next_fcfs_process(processes, process_count, queued);
        if (selected == -1) {
            return;
        }

        if (!queue_enqueue(ready_queue, selected)) {
            return;
        }

        queued[selected] = 1;
    }
}

static void print_timeline_row(int start_time, int end_time, const char *label)
{
    printf("%5d  %5d  %s\n", start_time, end_time, label);
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
    printf("PID  Arrival  CPU Burst  Completion  Waiting  Turnaround\n");
    printf("---  -------  ---------  ----------  -------  ----------\n");

    for (i = 0; i < process_count; i++) {
        total_waiting_time += waiting_times[i];
        total_turnaround_time += turnaround_times[i];

        printf("P%-3d %-8d %-10d %-11d %-8d %-10d\n",
               processes[i].pid,
               processes[i].arrival_time,
               processes[i].cpu_burst_time,
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
    ReadyQueue ready_queue;
    int completion_times[MAX_PROCESSES] = {0};
    int waiting_times[MAX_PROCESSES] = {0};
    int turnaround_times[MAX_PROCESSES] = {0};
    int current_time = 0;
    int process_index;

    if (process_count == 0) {
        printf("\nNo processes have been created yet.\n");
        return;
    }

    build_fcfs_queue(&ready_queue, processes, process_count);
    gantt_init(&gantt_chart);

    printf("\n[FCFS Scheduling]\n");
    printf("Tie-break rule: earlier arrival time, then smaller PID.\n");
    printf("TODO: I/O handling.\n\n");

    queue_print("FCFS Order", &ready_queue, processes);

    printf("\nExecution Timeline\n");
    printf("Start    End  Process\n");
    printf("-----  -----  -------\n");

    while (queue_dequeue(&ready_queue, &process_index)) {
        const Process *process = &processes[process_index];
        int start_time;
        int end_time;

        if (current_time < process->arrival_time) {
            print_timeline_row(current_time, process->arrival_time, "IDLE");
            gantt_add_segment(&gantt_chart, current_time, process->arrival_time, GANTT_IDLE_PID);
            current_time = process->arrival_time;
        }

        start_time = current_time;
        end_time = start_time + process->cpu_burst_time;

        printf("%5d  %5d  P%d\n", start_time, end_time, process->pid);
        gantt_add_segment(&gantt_chart, start_time, end_time, process->pid);

        completion_times[process_index] = end_time;
        turnaround_times[process_index] = end_time - process->arrival_time;
        waiting_times[process_index] = turnaround_times[process_index] - process->cpu_burst_time;

        current_time = end_time;
    }

    gantt_print(&gantt_chart);
    print_fcfs_metrics(processes,
                       process_count,
                       completion_times,
                       waiting_times,
                       turnaround_times);
}
