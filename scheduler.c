#include <stdio.h>

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

void scheduler_run_fcfs(void)
{
    const Process *processes = process_get_all();
    int process_count = process_get_count();
    ReadyQueue ready_queue;
    int current_time = 0;
    int process_index;

    if (process_count == 0) {
        printf("\nNo processes have been created yet.\n");
        return;
    }

    build_fcfs_queue(&ready_queue, processes, process_count);

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
            current_time = process->arrival_time;
        }

        start_time = current_time;
        end_time = start_time + process->cpu_burst_time;

        printf("%5d  %5d  P%d\n", start_time, end_time, process->pid);
        current_time = end_time;
    }
}
