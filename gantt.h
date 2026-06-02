#ifndef GANTT_H
#define GANTT_H

#define MAX_GANTT_SEGMENTS 200
#define GANTT_IDLE_PID -1

typedef struct {
    int start_time;
    int end_time;
    int pid;
} GanttSegment;

typedef struct {
    GanttSegment segments[MAX_GANTT_SEGMENTS];
    int count;
} GanttChart;

void gantt_init(GanttChart *chart);
int gantt_add_segment(GanttChart *chart, int start_time, int end_time, int pid);
void gantt_print_timeline(const GanttChart *chart);
void gantt_print(const GanttChart *chart);

#endif
