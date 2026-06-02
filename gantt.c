#include <stdio.h>

#include "gantt.h"

#define GANTT_CELL_WIDTH 8

static void print_segment_label(int pid)
{
    if (pid == GANTT_IDLE_PID) {
        printf("| %-*s", GANTT_CELL_WIDTH - 2, "IDLE");
        return;
    }

    printf("| P%-*d", GANTT_CELL_WIDTH - 3, pid);
}

void gantt_init(GanttChart *chart)
{
    chart->count = 0;
}

int gantt_add_segment(GanttChart *chart, int start_time, int end_time, int pid)
{
    GanttSegment *last;

    if (start_time >= end_time) {
        return 1;
    }

    if (chart->count > 0) {
        last = &chart->segments[chart->count - 1];
        /* Merge adjacent segments. Keep one long P1 run from printing as many tiny P1 blocks. */
        if (last->pid == pid && last->end_time == start_time) {
            last->end_time = end_time;
            return 1;
        }
    }

    if (chart->count >= MAX_GANTT_SEGMENTS) {
        return 0;
    }

    chart->segments[chart->count].start_time = start_time;
    chart->segments[chart->count].end_time = end_time;
    chart->segments[chart->count].pid = pid;
    chart->count++;

    return 1;
}

void gantt_print_timeline(const GanttChart *chart)
{
    int i;

    printf("\nExecution Timeline\n");

    if (chart->count == 0) {
        printf("(empty)\n");
        return;
    }

    printf("Start    End  Process\n");
    printf("-----  -----  -------\n");

    for (i = 0; i < chart->count; i++) {
        printf("%5d  %5d  ",
               chart->segments[i].start_time,
               chart->segments[i].end_time);

        if (chart->segments[i].pid == GANTT_IDLE_PID) {
            printf("IDLE\n");
        } else {
            printf("P%d\n", chart->segments[i].pid);
        }
    }
}

void gantt_print(const GanttChart *chart)
{
    int i;

    printf("\nGantt Chart\n");

    if (chart->count == 0) {
        printf("(empty)\n");
        return;
    }

    for (i = 0; i < chart->count; i++) {
        print_segment_label(chart->segments[i].pid);
    }
    printf("|\n");

    for (i = 0; i < chart->count; i++) {
        printf("%-*d", GANTT_CELL_WIDTH, chart->segments[i].start_time);
    }
    printf("%d\n", chart->segments[chart->count - 1].end_time);
}
