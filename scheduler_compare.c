#include <math.h>
#include <stdio.h>

#include "config.h"
#include "process.h"
#include "scheduler.h"
#include "scheduler_internal.h"

#define COMPARE_ALGORITHM_COUNT 6
#define METRIC_EPSILON 0.000001
#define STAT_MAX_RUNS 100000
#define STAT_MAX_SEED 1000000000

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

/////////// Statistics ///////////

/* One algorithm's metrics accumulated across many random workloads. */
typedef struct {
    const char *name;
    double sum_waiting;
    double sum_turnaround;
    double sum_response;
    double sum_utilization;
    double sum_throughput;
    double sumsq_waiting;
    double sumsq_turnaround;
    double min_waiting;
    double max_waiting;
    int best_waiting;
    int best_turnaround;
} StatRow;

/* Discard leftover input after reading one statistical-run parameter. */
static void stat_clear_input(void)
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

/* Read one integer parameter and reject values outside [min, max]. */
static int read_int_in_range(const char *prompt, int min, int max, int *value)
{
    int result;

    printf("%s", prompt);
    result = scanf("%d", value);
    if (result != 1) {
        printf("Invalid input. Please enter a number.\n");
        stat_clear_input();
        return 0;
    }

    stat_clear_input();

    if (*value < min) {
        printf("Value must be between %d and %d.\n", min, max);
        return 0;
    }

    if (*value > max) {
        printf("Value must be between %d and %d.\n", min, max);
        return 0;
    }

    return 1;
}

/* Reset every accumulator before the first workload. */
static void stat_init(StatRow rows[], int row_count)
{
    int i;

    for (i = 0; i < row_count; i++) {
        rows[i].name = 0;
        rows[i].sum_waiting = 0.0;
        rows[i].sum_turnaround = 0.0;
        rows[i].sum_response = 0.0;
        rows[i].sum_utilization = 0.0;
        rows[i].sum_throughput = 0.0;
        rows[i].sumsq_waiting = 0.0;
        rows[i].sumsq_turnaround = 0.0;
        rows[i].min_waiting = HUGE_VAL;
        rows[i].max_waiting = -1.0;
        rows[i].best_waiting = 0;
        rows[i].best_turnaround = 0;
    }
}

/* Fold one workload's results into the running accumulators. */
static void stat_accumulate(StatRow acc[],
                            const CompareRow run_rows[],
                            int row_count)
{
    double best_waiting;
    double best_turnaround;
    int i;

    best_waiting = run_rows[0].result.average_waiting_time;
    best_turnaround = run_rows[0].result.average_turnaround_time;
    for (i = 1; i < row_count; i++) {
        if (run_rows[i].result.average_waiting_time < best_waiting) {
            best_waiting = run_rows[i].result.average_waiting_time;
        }

        if (run_rows[i].result.average_turnaround_time < best_turnaround) {
            best_turnaround = run_rows[i].result.average_turnaround_time;
        }
    }

    for (i = 0; i < row_count; i++) {
        double waiting = run_rows[i].result.average_waiting_time;
        double turnaround = run_rows[i].result.average_turnaround_time;

        if (acc[i].name == 0) {
            acc[i].name = run_rows[i].name;
        }

        acc[i].sum_waiting += waiting;
        acc[i].sum_turnaround += turnaround;
        acc[i].sum_response += run_rows[i].result.average_response_time;
        acc[i].sum_utilization += run_rows[i].result.cpu_utilization;
        acc[i].sum_throughput += run_rows[i].result.throughput;
        acc[i].sumsq_waiting += waiting * waiting;
        acc[i].sumsq_turnaround += turnaround * turnaround;

        if (waiting < acc[i].min_waiting) {
            acc[i].min_waiting = waiting;
        }

        if (waiting > acc[i].max_waiting) {
            acc[i].max_waiting = waiting;
        }

        if (same_metric(waiting, best_waiting)) {
            acc[i].best_waiting++;
        }

        if (same_metric(turnaround, best_turnaround)) {
            acc[i].best_turnaround++;
        }
    }
}

/* Population mean of an accumulated sum. */
static double stat_mean(double sum, int n)
{
    if (n <= 0) {
        return 0.0;
    }

    return sum / n;
}

/* Population standard deviation from a sum and a sum of squares. */
static double stat_std(double sum, double sum_sq, int n)
{
    double mean;
    double variance;

    if (n <= 0) {
        return 0.0;
    }

    mean = sum / n;
    variance = sum_sq / n - mean * mean;
    if (variance < 0.0) {
        variance = 0.0;
    }

    return sqrt(variance);
}

/* Print the mean of every metric for each algorithm. */
static void stat_print_means(const StatRow acc[], int row_count, int num_runs)
{
    int i;

    printf("\nAverage Metrics over %d Workloads\n", num_runs);
    printf("Algorithm                     Avg Wait  Avg Turn  Avg Resp  CPU Util  Throughput\n");
    printf("----------------------------  --------  --------  --------  --------  ----------\n");

    for (i = 0; i < row_count; i++) {
        printf("%-28s  %8.2f  %8.2f  %8.2f  %7.2f%%  %10.4f\n",
               acc[i].name,
               stat_mean(acc[i].sum_waiting, num_runs),
               stat_mean(acc[i].sum_turnaround, num_runs),
               stat_mean(acc[i].sum_response, num_runs),
               stat_mean(acc[i].sum_utilization, num_runs),
               stat_mean(acc[i].sum_throughput, num_runs));
    }
}

/* Print spread and win-rate for the two required metrics. */
static void stat_print_consistency(const StatRow acc[],
                                   int row_count,
                                   int num_runs)
{
    int i;

    printf("\nConsistency over %d Workloads (lower waiting/turnaround is better)\n",
           num_runs);
    printf("Algorithm                     Wait SD  Wait Min  Wait Max  Best-Wait  Turn SD  Best-Turn\n");
    printf("----------------------------  -------  --------  --------  ---------  -------  ---------\n");

    for (i = 0; i < row_count; i++) {
        double waiting_win = 100.0 * acc[i].best_waiting / num_runs;
        double turnaround_win = 100.0 * acc[i].best_turnaround / num_runs;

        printf("%-28s  %7.2f  %8.2f  %8.2f  %7.1f%%  %7.2f  %7.1f%%\n",
               acc[i].name,
               stat_std(acc[i].sum_waiting, acc[i].sumsq_waiting, num_runs),
               acc[i].min_waiting,
               acc[i].max_waiting,
               waiting_win,
               stat_std(acc[i].sum_turnaround, acc[i].sumsq_turnaround, num_runs),
               turnaround_win);
    }

    printf("Best-Wait / Best-Turn = share of workloads where the algorithm reached");
    printf(" the best value (ties counted for each).\n");
}

/* Print which algorithm wins each metric averaged across all workloads. */
static void stat_print_best(const StatRow acc[], int row_count, int num_runs)
{
    int i;
    int best_waiting = 0;
    int best_turnaround = 0;
    int best_response = 0;
    int best_utilization = 0;
    int best_throughput = 0;

    for (i = 1; i < row_count; i++) {
        if (acc[i].sum_waiting < acc[best_waiting].sum_waiting) {
            best_waiting = i;
        }

        if (acc[i].sum_turnaround < acc[best_turnaround].sum_turnaround) {
            best_turnaround = i;
        }

        if (acc[i].sum_response < acc[best_response].sum_response) {
            best_response = i;
        }

        if (acc[i].sum_utilization > acc[best_utilization].sum_utilization) {
            best_utilization = i;
        }

        if (acc[i].sum_throughput > acc[best_throughput].sum_throughput) {
            best_throughput = i;
        }
    }

    printf("\nBest on Average\n");
    printf("Lowest Avg Waiting Time:    %s (%.2f)\n",
           acc[best_waiting].name,
           stat_mean(acc[best_waiting].sum_waiting, num_runs));
    printf("Lowest Avg Turnaround Time: %s (%.2f)\n",
           acc[best_turnaround].name,
           stat_mean(acc[best_turnaround].sum_turnaround, num_runs));
    printf("Lowest Avg Response Time:   %s (%.2f)\n",
           acc[best_response].name,
           stat_mean(acc[best_response].sum_response, num_runs));
    printf("Highest CPU Utilization:    %s (%.2f%%)\n",
           acc[best_utilization].name,
           stat_mean(acc[best_utilization].sum_utilization, num_runs));
    printf("Highest Throughput:         %s (%.4f)\n",
           acc[best_throughput].name,
           stat_mean(acc[best_throughput].sum_throughput, num_runs));
}

/* Run many random workloads and report averaged scheduling metrics. */
void scheduler_compare_statistics(void)
{
    Process backup[MAX_PROCESSES];
    StatRow acc[COMPARE_ALGORITHM_COUNT];
    const Process *current;
    int process_limit = config_get()->process_limit;
    int backup_count;
    int num_runs;
    int proc_count;
    int seed;
    int time_quantum;
    int run;
    int i;

    if (!read_int_in_range("\nNumber of random workloads (1-100000): ",
                           1, STAT_MAX_RUNS, &num_runs)) {
        return;
    }

    printf("Processes per workload (1-%d): ", process_limit);
    if (!read_int_in_range("", 1, process_limit, &proc_count)) {
        return;
    }

    if (!read_int_in_range("Random seed (0 for time-based): ",
                           0, STAT_MAX_SEED, &seed)) {
        return;
    }

    time_quantum = scheduler_read_time_quantum();
    if (time_quantum == 0) {
        return;
    }

    /* Save the user's current process set so the batch run is non-destructive. */
    current = process_get_all();
    backup_count = process_get_count();
    for (i = 0; i < backup_count; i++) {
        backup[i] = current[i];
    }

    process_seed_random(seed);
    stat_init(acc, COMPARE_ALGORITHM_COUNT);

    for (run = 0; run < num_runs; run++) {
        CompareRow run_rows[COMPARE_ALGORITHM_COUNT];
        int row_count;

        process_generate_random(proc_count);
        row_count = collect_results(run_rows, time_quantum);
        if (row_count != COMPARE_ALGORITHM_COUNT) {
            printf("\nA workload run failed; statistics aborted.\n");
            process_restore_set(backup, backup_count);
            return;
        }

        stat_accumulate(acc, run_rows, row_count);
    }

    process_restore_set(backup, backup_count);

    printf("\n[Statistical Scheduling Comparison]\n");
    if (seed == 0) {
        printf("Workloads: %d | Processes each: %d | RR quantum: %d | Seed: time-based\n",
               num_runs, proc_count, time_quantum);
    } else {
        printf("Workloads: %d | Processes each: %d | RR quantum: %d | Seed: %d\n",
               num_runs, proc_count, time_quantum, seed);
    }
    printf("Each workload is newly randomized; all six algorithms run on the same workload.\n");

    stat_print_means(acc, COMPARE_ALGORITHM_COUNT, num_runs);
    stat_print_consistency(acc, COMPARE_ALGORITHM_COUNT, num_runs);
    stat_print_best(acc, COMPARE_ALGORITHM_COUNT, num_runs);
}
