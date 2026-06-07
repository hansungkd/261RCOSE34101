#include <stdio.h>

#include "config.h"
#include "gantt.h"
#include "priority_queue.h"
#include "queue.h"

static SimulatorConfig config = {
    MAX_PROCESSES,
    9,
    10,
    MAX_IO_EVENTS,
    5,
    5
};

/* Return the current runtime simulator configuration. */
const SimulatorConfig *config_get(void)
{
    return &config;
}

/* Print compile-time limits and runtime configuration values. */
void config_print(void)
{
    printf("\nCompile-Time Limits\n");
    printf("-------------------\n");
    printf("MAX_PROCESSES: %d\n", MAX_PROCESSES);
    printf("MAX_IO_EVENTS: %d\n", MAX_IO_EVENTS);
    printf("QUEUE_CAPACITY: %d\n", QUEUE_CAPACITY);
    printf("PQ_CAPACITY: %d\n", PQ_CAPACITY);
    printf("GANTT_INITIAL_CAPACITY: %d, grows dynamically\n",
           GANTT_INITIAL_CAPACITY);

    printf("\nRuntime Configuration\n");
    printf("---------------------\n");
    printf("Process limit for random creation: %d (compile max %d)\n",
           config.process_limit,
           MAX_PROCESSES);
    printf("Arrival time range: 0-%d\n", config.arrival_time_max);
    printf("CPU burst time range: 1-%d\n", config.cpu_burst_max);
    printf("I/O event count range: 0-%d (compile max %d)\n",
           config.io_event_limit,
           MAX_IO_EVENTS);
    printf("I/O duration range per event: 1-%d\n", config.io_duration_max);
    printf("Priority range: 1-%d, smaller number means higher priority\n",
           config.priority_max);
}

/* Set the maximum number of processes allowed during random creation. */
int config_set_process_limit(int value)
{
    if (value < 1) {
        return 0;
    }

    if (value > MAX_PROCESSES) {
        return 0;
    }

    config.process_limit = value;
    return 1;
}

/* Set the highest random arrival time. */
int config_set_arrival_time_max(int value)
{
    if (value < 0) {
        return 0;
    }

    config.arrival_time_max = value;
    return 1;
}

/* Set the highest random CPU burst time. */
int config_set_cpu_burst_max(int value)
{
    if (value < 1) {
        return 0;
    }

    config.cpu_burst_max = value;
    return 1;
}

/* Set the random I/O event count limit, bounded by the struct array size. */
int config_set_io_event_limit(int value)
{
    if (value < 0) {
        return 0;
    }

    if (value > MAX_IO_EVENTS) {
        return 0;
    }

    config.io_event_limit = value;
    return 1;
}

/* Set the highest random I/O duration per event. */
int config_set_io_duration_max(int value)
{
    if (value < 1) {
        return 0;
    }

    config.io_duration_max = value;
    return 1;
}

/* Set the highest random priority value. */
int config_set_priority_max(int value)
{
    if (value < 1) {
        return 0;
    }

    config.priority_max = value;
    return 1;
}
