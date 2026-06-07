#ifndef GANTT_H
#define GANTT_H

#define GANTT_INITIAL_CAPACITY 64
#define GANTT_IDLE_PID -1

typedef struct {
    int start_time;
    int end_time;
    int pid;
} GanttSegment;

typedef struct {
    GanttSegment *segments;
    int count;
    int capacity;
    int truncated;
} GanttChart;

void gantt_init(GanttChart *chart);
void gantt_free(GanttChart *chart);
int gantt_add_segment(GanttChart *chart, int start_time, int end_time, int pid);
void gantt_print_timeline(const GanttChart *chart);
void gantt_print(const GanttChart *chart);

#endif
