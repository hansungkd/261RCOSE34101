#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "process.h"

static Process processes[MAX_PROCESSES] = {
    {1, 0, 8, 3, 2},
    {2, 1, 4, 2, 1},
    {3, 2, 9, 0, 3}
};

static int process_count = 3;

static void clear_input_buffer(void)
{
    int ch;

    while ((ch = getchar()) != '\n' && ch != EOF) {
        /* discard remaining input */
    }
}

static int random_between(int min, int max)
{
    return min + rand() % (max - min + 1);
}

static void seed_random_once(void)
{
    static int seeded = 0;

    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }
}

void process_create_random(void)
{
    int count;
    int i;

    printf("\nNumber of processes (1-%d): ", MAX_PROCESSES);
    if (scanf("%d", &count) != 1) {
        printf("Invalid process count.\n");
        clear_input_buffer();
        return;
    }

    if (count < 1 || count > MAX_PROCESSES) {
        printf("Process count must be between 1 and %d.\n", MAX_PROCESSES);
        return;
    }

    seed_random_once();
    process_count = count;

    for (i = 0; i < process_count; i++) {
        processes[i].pid = i + 1;
        processes[i].arrival_time = random_between(0, 9);
        processes[i].cpu_burst_time = random_between(1, 10);
        processes[i].io_burst_time = random_between(0, 5);
        processes[i].priority = random_between(1, 5);
    }

    printf("Created %d random processes.\n", process_count);
}

void process_print_all(void)
{
    int i;

    if (process_count == 0) {
        printf("\nNo processes have been created yet.\n");
        return;
    }

    printf("\nCurrent Process Set\n");
    printf("PID  Arrival  CPU Burst  I/O Burst  Priority\n");
    printf("---  -------  ---------  ---------  --------\n");

    for (i = 0; i < process_count; i++) {
        printf("P%-3d %-8d %-10d %-10d %-8d\n",
               processes[i].pid,
               processes[i].arrival_time,
               processes[i].cpu_burst_time,
               processes[i].io_burst_time,
               processes[i].priority);
    }
}

int process_get_count(void)
{
    return process_count;
}

const Process *process_get_all(void)
{
    return processes;
}

const Process *process_get_at(int index)
{
    if (index < 0 || index >= process_count) {
        return NULL;
    }

    return &processes[index];
}
