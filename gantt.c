#include <stdio.h>
#include <stdlib.h>

#include "gantt.h"

#define GANTT_CELL_WIDTH 8
#define TIMELINE_PAGE_ROWS 20
#define GANTT_SEGMENTS_PER_ROW 8
#define GANTT_ROWS_PER_PAGE 4

/* Pause long output so the user can read one screen at a time. */
static void wait_for_enter(void)
{
    int ch;

    printf("\nPress Enter to continue...");
    fflush(stdout);

    ch = getchar();
    while (ch != '\n') {
        if (ch == EOF) {
            printf("\n");
            return;
        }

        ch = getchar();
    }

    printf("\n");
}

/* Make room for one more segment. The array grows as the chart gets longer. */
static int ensure_capacity(GanttChart *chart)
{
    GanttSegment *new_segments;
    int new_capacity;

    if (chart->count < chart->capacity) {
        return 1;
    }

    if (chart->capacity == 0) {
        new_capacity = GANTT_INITIAL_CAPACITY;
    } else {
        new_capacity = chart->capacity * 2;
    }

    new_segments = realloc(chart->segments,
                           sizeof(GanttSegment) * new_capacity);
    if (new_segments == NULL) {
        chart->truncated = 1;
        return 0;
    }

    chart->segments = new_segments;
    chart->capacity = new_capacity;
    return 1;
}

/* Print one Gantt cell label for either a process or IDLE time. */
static void print_segment_label(int pid)
{
    if (pid == GANTT_IDLE_PID) {
        printf("| %-*s", GANTT_CELL_WIDTH - 2, "IDLE");
        return;
    }

    printf("| P%-*d", GANTT_CELL_WIDTH - 3, pid);
}

/* Start an empty Gantt chart. */
void gantt_init(GanttChart *chart)
{
    chart->segments = NULL;
    chart->count = 0;
    chart->capacity = 0;
    chart->truncated = 0;
}

/* Release memory used by a dynamically grown Gantt chart. */
void gantt_free(GanttChart *chart)
{
    free(chart->segments);
    chart->segments = NULL;
    chart->count = 0;
    chart->capacity = 0;
    chart->truncated = 0;
}

/* Add a segment, merging adjacent segments for the same process. */
int gantt_add_segment(GanttChart *chart, int start_time, int end_time, int pid)
{
    GanttSegment *last;

    if (chart == NULL) {
        return 0;
    }

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

    if (ensure_capacity(chart) == 0) {
        return 0;
    }

    chart->segments[chart->count].start_time = start_time;
    chart->segments[chart->count].end_time = end_time;
    chart->segments[chart->count].pid = pid;
    chart->count++;

    return 1;
}

/* Print start/end rows for each Gantt segment. */
void gantt_print_timeline(const GanttChart *chart)
{
    int i;
    int rows_on_page = 0;

    printf("\nExecution Timeline\n");

    if (chart->count == 0) {
        printf("(empty)\n");
        return;
    }

    if (chart->truncated) {
        printf("Warning: Gantt chart output is incomplete because memory allocation failed.\n");
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

        rows_on_page++;
        if (i < chart->count - 1) {
            if (rows_on_page == TIMELINE_PAGE_ROWS) {
                wait_for_enter();
                rows_on_page = 0;
            }
        }
    }

    wait_for_enter();
}

/* Print the compact bar-style Gantt chart. */
void gantt_print(const GanttChart *chart)
{
    int row_start = 0;
    int rows_on_page = 0;

    printf("\nGantt Chart\n");

    if (chart->count == 0) {
        printf("(empty)\n");
        return;
    }

    while (row_start < chart->count) {
        int row_end;
        int i;

        row_end = row_start + GANTT_SEGMENTS_PER_ROW;
        if (row_end > chart->count) {
            row_end = chart->count;
        }

        for (i = row_start; i < row_end; i++) {
            print_segment_label(chart->segments[i].pid);
        }
        printf("|\n");

        for (i = row_start; i < row_end; i++) {
            printf("%-*d", GANTT_CELL_WIDTH, chart->segments[i].start_time);
        }
        printf("%d\n", chart->segments[row_end - 1].end_time);

        row_start = row_end;
        rows_on_page++;

        if (row_start < chart->count) {
            if (rows_on_page == GANTT_ROWS_PER_PAGE) {
                wait_for_enter();
                rows_on_page = 0;
            }
        }
    }

    wait_for_enter();
}
