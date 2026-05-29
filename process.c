#include <stdio.h>

#include "process.h"

static Process processes[MAX_PROCESSES] = {
    {1, 0, 8, 3, 2},
    {2, 1, 4, 2, 1},
    {3, 2, 9, 0, 3}
};

static int process_count = 3;

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
