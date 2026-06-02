#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "process.h"

/* Default data */
static Process processes[MAX_PROCESSES] = {
    {1, 0, 8, 3, 2, 1, {{4, 3}}},
    {2, 1, 4, 2, 1, 1, {{2, 2}}},
    {3, 2, 9, 0, 3, 0, {{0, 0}}}
};

static int process_count = 3;

/* Discard the rest of the current input line after bad scanf input. */
static void clear_input_buffer(void)
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

/* Return a random integer in the inclusive range [min, max]. */
static int random_between(int min, int max)
{
    int range;
    int random_value;

    range = max - min + 1;
    random_value = rand() % range;

    return min + random_value;
}

/* Remove all I/O events from one process before generating new ones. */
static void clear_io(Process *process)
{
    int i;

    process->io_burst_time = 0;
    process->io_event_count = 0;

    for (i = 0; i < MAX_IO_EVENTS; i++) {
        process->io_events[i].trigger_time = 0;
        process->io_events[i].duration = 0;
    }
}

/* Seed rand() once for this program run. */
static void seed_random_once(void)
{
    static int seeded = 0;

    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }
}

/* Check whether an I/O trigger time is already used by this process. */
static int trigger_used(const Process *process, int trigger_time)
{
    int i;

    for (i = 0; i < process->io_event_count; i++) {
        if (process->io_events[i].trigger_time == trigger_time) {
            return 1;
        }
    }

    return 0;
}

/* Keep I/O events sorted by their CPU trigger time. */
static void sort_io(Process *process)
{
    int i;
    int j;

    for (i = 0; i < process->io_event_count - 1; i++) {
        for (j = i + 1; j < process->io_event_count; j++) {
            int current_trigger;
            int next_trigger;

            current_trigger = process->io_events[i].trigger_time;
            next_trigger = process->io_events[j].trigger_time;

            if (next_trigger < current_trigger) {
                IoEvent temp = process->io_events[i];
                process->io_events[i] = process->io_events[j];
                process->io_events[j] = temp;
            }
        }
    }
}

/* Generate random I/O event count, trigger times, and durations. */
static void randomize_io(Process *process)
{
    int max_event_count;
    int i;

    /* Randomize I/O count, trigger points, and durations. */
    clear_io(process);

    if (process->cpu_burst_time <= 1) {
        return;
    }

    max_event_count = process->cpu_burst_time - 1;
    if (max_event_count > MAX_IO_EVENTS) {
        max_event_count = MAX_IO_EVENTS;
    }

    process->io_event_count = random_between(0, max_event_count);

    for (i = 0; i < process->io_event_count; i++) {
        int trigger_time; // when to trigger io (executed CPU time)
        int duplicate_trigger;

        duplicate_trigger = 1;
        while (duplicate_trigger) {
            trigger_time = random_between(1, process->cpu_burst_time - 1);
            duplicate_trigger = trigger_used(process, trigger_time);
        }

        process->io_events[i].trigger_time = trigger_time;
        process->io_events[i].duration = random_between(1, 5);
        process->io_burst_time += process->io_events[i].duration;
    }

    sort_io(process);
}

/* Print one process's I/O events as trigger:duration values. */
static void print_io(const Process *process)
{
    int i;

    /* Print as trigger:duration */
    if (process->io_event_count == 0) {
        printf("-");
        return;
    }

    for (i = 0; i < process->io_event_count; i++) {
        printf("%d:%d",
               process->io_events[i].trigger_time,
               process->io_events[i].duration);

        if (i < process->io_event_count - 1) {
            printf(", ");
        }
    }
}

/* Replace the current process set with randomly generated processes. */
void process_create_random(void)
{
    int count;
    int i;
    int result;

    printf("\nNumber of processes (1-%d): ", MAX_PROCESSES);
    result = scanf("%d", &count);
    if (result != 1) {
        printf("Invalid process count.\n");
        clear_input_buffer();
        return;
    }

    if (count < 1) {
        printf("Process count must be between 1 and %d.\n", MAX_PROCESSES);
        return;
    }

    if (count > MAX_PROCESSES) {
        printf("Process count must be between 1 and %d.\n", MAX_PROCESSES);
        return;
    }

    seed_random_once();
    process_count = count;

    for (i = 0; i < process_count; i++) {
        processes[i].pid = i + 1;
        processes[i].arrival_time = random_between(0, 9);
        processes[i].cpu_burst_time = random_between(1, 10);
        processes[i].priority = random_between(1, 5);
        randomize_io(&processes[i]);
    }

    printf("Created %d random processes.\n", process_count);
}

/* Print the process table currently used by scheduler runs. */
void process_print_all(void)
{
    int i;

    if (process_count == 0) {
        printf("\nNo processes have been created yet.\n");
        return;
    }

    printf("\nCurrent Process Set\n");
    printf("PID  Arrival  CPU Burst  I/O Total  Priority  I/O Events\n");
    printf("---  -------  ---------  ---------  --------  ----------\n");

    for (i = 0; i < process_count; i++) {
        printf("P%-3d %-8d %-10d %-10d %-9d ",
               processes[i].pid,
               processes[i].arrival_time,
               processes[i].cpu_burst_time,
               processes[i].io_burst_time,
               processes[i].priority);
        print_io(&processes[i]);
        printf("\n");
    }
}

/* Return how many processes are currently loaded. */
int process_get_count(void)
{
    return process_count;
}

/* Return the current process array without copying it. */
const Process *process_get_all(void)
{
    return processes;
}

/* Return one process by array index, or NULL for an invalid index. */
const Process *process_get_at(int index)
{
    if (index < 0) {
        return NULL;
    }

    if (index >= process_count) {
        return NULL;
    }

    return &processes[index];
}
