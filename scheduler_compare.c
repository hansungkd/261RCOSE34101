#include <stdio.h>

#include "process.h"
#include "scheduler.h"
#include "scheduler_internal.h"

#define COMPARE_ALGORITHM_COUNT 6

typedef struct {
    const char *name;
    ScheduleResult result;
} CompareRow;

/* Copy one algorithm result into the comparison table rows. */
static void save_row(CompareRow rows[],
                     int *row_count,
                     const char *name,
                     const ScheduleResult *result)
{
    if (*row_count >= COMPARE_ALGORITHM_COUNT) {
        return;
    }

    rows[*row_count].name = name;
    rows[*row_count].result = *result;
    (*row_count)++;
}

/* Run every algorithm on the current process set and collect only metrics. */
static int collect_results(CompareRow rows[], int time_quantum)
{
    ScheduleResult result;
    int row_count = 0;
    int ran;

    ran = scheduler_fcfs_result(&result);
    if (ran) {
        save_row(rows, &row_count, "FCFS", &result);
    }

    ran = scheduler_nonpreemptive_sjf_result(&result);
    if (ran) {
        save_row(rows, &row_count, "Non-Preemptive SJF", &result);
    }

    ran = scheduler_preemptive_sjf_result(&result);
    if (ran) {
        save_row(rows, &row_count, "Preemptive SJF", &result);
    }

    ran = scheduler_nonpreemptive_priority_result(&result);
    if (ran) {
        save_row(rows, &row_count, "Non-Preemptive Priority", &result);
    }

    ran = scheduler_preemptive_priority_result(&result);
    if (ran) {
        save_row(rows, &row_count, "Preemptive Priority", &result);
    }

    ran = scheduler_round_robin_result(&result, time_quantum);
    if (ran) {
        save_row(rows, &row_count, "Round Robin", &result);
    }

    return row_count;
}

/* Print one compact table for comparing the collected metrics. */
static void print_table(const CompareRow rows[], int row_count)
{
    int i;

    printf("\nComparison Table\n");
    printf("Algorithm                     Avg Waiting  Avg Turnaround  Finish Time\n");
    printf("----------------------------  -----------  --------------  -----------\n");

    for (i = 0; i < row_count; i++) {
        printf("%-28s  %11.2f  %14.2f  %11d\n",
               rows[i].name,
               rows[i].result.average_waiting_time,
               rows[i].result.average_turnaround_time,
               rows[i].result.finish_time);
    }
}

/* Print the best algorithms by the two main project metrics. */
static void print_best(const CompareRow rows[], int row_count)
{
    int i;
    int best_waiting = 0;
    int best_turnaround = 0;

    for (i = 1; i < row_count; i++) {
        if (rows[i].result.average_waiting_time <
            rows[best_waiting].result.average_waiting_time) {
            best_waiting = i;
        }

        if (rows[i].result.average_turnaround_time <
            rows[best_turnaround].result.average_turnaround_time) {
            best_turnaround = i;
        }
    }

    printf("\nBest Average Waiting Time: %s (%.2f)\n",
           rows[best_waiting].name,
           rows[best_waiting].result.average_waiting_time);
    printf("Best Average Turnaround Time: %s (%.2f)\n",
           rows[best_turnaround].name,
           rows[best_turnaround].result.average_turnaround_time);
}

/* Compare every implemented scheduler using the same current process set. */
void scheduler_compare_results(void)
{
    CompareRow rows[COMPARE_ALGORITHM_COUNT];
    int process_count = process_get_count();
    int time_quantum;
    int row_count;

    if (process_count == 0) {
        printf("\nNo processes have been created yet.\n");
        return;
    }

    time_quantum = scheduler_read_time_quantum();
    if (time_quantum == 0) {
        return;
    }

    row_count = collect_results(rows, time_quantum);
    if (row_count == 0) {
        printf("\nNo comparison result was created.\n");
        return;
    }

    printf("\n[Scheduling Result Comparison]\n");
    printf("All algorithms use the same current process set.\n");
    printf("Waiting time counts only time spent in Ready Queue.\n");
    printf("Round Robin time quantum: %d\n", time_quantum);

    process_print_all();
    print_table(rows, row_count);
    print_best(rows, row_count);
}
