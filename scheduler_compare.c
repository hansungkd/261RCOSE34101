#include <stdio.h>

#include "process.h"
#include "scheduler.h"
#include "scheduler_internal.h"

#define COMPARE_ALGORITHM_COUNT 6
#define METRIC_EPSILON 0.000001

typedef enum {
    METRIC_WAITING,
    METRIC_TURNAROUND,
    METRIC_RESPONSE,
    METRIC_UTILIZATION,
    METRIC_THROUGHPUT
} MetricType;

typedef struct {
    const char *name;
    ScheduleResult result;
} CompareRow;

/* Return one metric value from a comparison row. */
static double metric_value(const CompareRow *row, MetricType metric)
{
    if (metric == METRIC_WAITING) {
        return row->result.average_waiting_time;
    }

    if (metric == METRIC_TURNAROUND) {
        return row->result.average_turnaround_time;
    }

    if (metric == METRIC_RESPONSE) {
        return row->result.average_response_time;
    }

    if (metric == METRIC_UTILIZATION) {
        return row->result.cpu_utilization;
    }

    return row->result.throughput;
}

/* Compare doubles with a small tolerance for printed metric ties. */
static int same_metric(double left, double right)
{
    double difference;

    difference = left - right;
    if (difference < 0.0) {
        difference = -difference;
    }

    return difference < METRIC_EPSILON;
}

/* Print every algorithm tied for one best metric value. */
static void print_best_line(const char *label,
                            const CompareRow rows[],
                            int row_count,
                            int best_index,
                            MetricType metric,
                            int precision,
                            const char *suffix)
{
    double best_value = metric_value(&rows[best_index], metric);
    int i;
    int printed = 0;
    int tied_count = 0;

    printf("%s: ", label);

    for (i = 0; i < row_count; i++) {
        if (same_metric(metric_value(&rows[i], metric), best_value)) {
            tied_count++;
        }
    }

    if (tied_count == row_count) {
        printf("All algorithms");
        if (precision == 4) {
            printf(" (%.4f%s)\n", best_value, suffix);
        } else {
            printf(" (%.2f%s)\n", best_value, suffix);
        }
        return;
    }

    for (i = 0; i < row_count; i++) {
        if (!same_metric(metric_value(&rows[i], metric), best_value)) {
            continue;
        }

        if (printed) {
            printf(", ");
        }

        printf("%s", rows[i].name);
        printed = 1;
    }

    if (precision == 4) {
        printf(" (%.4f%s)\n", best_value, suffix);
    } else {
        printf(" (%.2f%s)\n", best_value, suffix);
    }
}

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
    printf("Algorithm                     Avg Wait  Avg Turn  Avg Resp  CPU Util  Throughput  Finish\n");
    printf("----------------------------  --------  --------  --------  --------  ----------  ------\n");

    for (i = 0; i < row_count; i++) {
        printf("%-28s  %8.2f  %8.2f  %8.2f  %7.2f%%  %10.4f  %6d\n",
               rows[i].name,
               rows[i].result.average_waiting_time,
               rows[i].result.average_turnaround_time,
               rows[i].result.average_response_time,
               rows[i].result.cpu_utilization,
               rows[i].result.throughput,
               rows[i].result.finish_time);
    }
}

/* Print the best algorithms for each comparison metric. */
static void print_best(const CompareRow rows[], int row_count)
{
    int i;
    int best_waiting = 0;
    int best_turnaround = 0;
    int best_response = 0;
    int best_utilization = 0;
    int best_throughput = 0;

    for (i = 1; i < row_count; i++) {
        if (rows[i].result.average_waiting_time <
            rows[best_waiting].result.average_waiting_time) {
            best_waiting = i;
        }

        if (rows[i].result.average_turnaround_time <
            rows[best_turnaround].result.average_turnaround_time) {
            best_turnaround = i;
        }

        if (rows[i].result.average_response_time <
            rows[best_response].result.average_response_time) {
            best_response = i;
        }

        if (rows[i].result.cpu_utilization >
            rows[best_utilization].result.cpu_utilization) {
            best_utilization = i;
        }

        if (rows[i].result.throughput >
            rows[best_throughput].result.throughput) {
            best_throughput = i;
        }
    }

    printf("\nBest Metrics\n");
    print_best_line("Average Waiting Time",
                    rows,
                    row_count,
                    best_waiting,
                    METRIC_WAITING,
                    2,
                    "");
    print_best_line("Average Turnaround Time",
                    rows,
                    row_count,
                    best_turnaround,
                    METRIC_TURNAROUND,
                    2,
                    "");
    print_best_line("Average Response Time",
                    rows,
                    row_count,
                    best_response,
                    METRIC_RESPONSE,
                    2,
                    "");
    print_best_line("CPU Utilization",
                    rows,
                    row_count,
                    best_utilization,
                    METRIC_UTILIZATION,
                    2,
                    "%");
    print_best_line("Throughput",
                    rows,
                    row_count,
                    best_throughput,
                    METRIC_THROUGHPUT,
                    4,
                    "");
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
