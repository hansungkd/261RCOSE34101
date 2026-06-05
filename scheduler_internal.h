#ifndef SCHEDULER_INTERNAL_H
#define SCHEDULER_INTERNAL_H

#include "gantt.h"
#include "priority_queue.h"
#include "process.h"
#include "queue.h"

typedef struct {
    int arrived;
    int completed;
    int remaining_cpu_time;
    int executed_cpu_time;
    int io_remaining_time;
    int next_io_event;
    int ready_order;
} ScheduleState;

typedef struct {
    int completion_times[MAX_PROCESSES];
    int waiting_times[MAX_PROCESSES];
    int turnaround_times[MAX_PROCESSES];
    double average_waiting_time;
    double average_turnaround_time;
    int finish_time;
} ScheduleResult;

int scheduler_arrival_precedes(const Process *candidate,
                               const Process *current);
int scheduler_read_time_quantum(void);
void scheduler_reset_states(ScheduleState states[],
                            const Process processes[],
                            int process_count);
int scheduler_next_arrival(const Process processes[],
                           int process_count,
                           const ScheduleState states[],
                           int current_time);
void scheduler_add_gantt(GanttChart *gantt_chart,
                         int start_time,
                         int end_time,
                         int pid);
void scheduler_fifo_admit(ReadyQueue *ready_queue,
                          const Process processes[],
                          int process_count,
                          ScheduleState states[],
                          int current_time);
void scheduler_fifo_count_wait(const ReadyQueue *ready_queue,
                               int waiting_times[]);
void scheduler_fifo_run_io(WaitingQueue *waiting_queue,
                           ReadyQueue *ready_queue,
                           const Process processes[],
                           ScheduleState states[],
                           int waiting_count,
                           int current_time,
                           int *completed_count,
                           int completion_times[],
                           int turnaround_times[]);
void scheduler_pq_admit(PriorityQueue *ready_queue,
                        const Process processes[],
                        int process_count,
                        ScheduleState states[],
                        int current_time,
                        int *ready_order);
void scheduler_pq_count_wait(const PriorityQueue *ready_queue,
                             int waiting_times[]);
void scheduler_pq_run_io(WaitingQueue *waiting_queue,
                         PriorityQueue *ready_queue,
                         const Process processes[],
                         ScheduleState states[],
                         int waiting_count,
                         int current_time,
                         int *ready_order,
                         int *completed_count,
                         int completion_times[],
                         int turnaround_times[]);
int scheduler_io_due(const Process *process, const ScheduleState *state);
void scheduler_start_io(WaitingQueue *waiting_queue,
                        const Process *process,
                        ScheduleState states[],
                        int process_index);
void scheduler_finish_process(const Process *process,
                              ScheduleState *state,
                              int process_index,
                              int completion_time,
                              int *completed_count,
                              int *completion_times,
                              int *turnaround_times);
void scheduler_print_metrics(const char *algorithm_name,
                             const Process processes[],
                             int process_count,
                             const int completion_times[],
                             const int waiting_times[],
                             const int turnaround_times[]);
void scheduler_save_result(ScheduleResult *result,
                           int process_count,
                           int finish_time,
                           const int completion_times[],
                           const int waiting_times[],
                           const int turnaround_times[]);

int scheduler_fcfs_result(ScheduleResult *result);
int scheduler_nonpreemptive_sjf_result(ScheduleResult *result);
int scheduler_preemptive_sjf_result(ScheduleResult *result);
int scheduler_nonpreemptive_priority_result(ScheduleResult *result);
int scheduler_preemptive_priority_result(ScheduleResult *result);
int scheduler_round_robin_result(ScheduleResult *result, int time_quantum);

#endif
