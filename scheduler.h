#ifndef SCHEDULER_H
#define SCHEDULER_H

void scheduler_run_fcfs(void);
void scheduler_run_nonpreemptive_sjf(void);
void scheduler_run_preemptive_sjf(void);
void scheduler_run_nonpreemptive_priority(void);
void scheduler_run_preemptive_priority(void);
void scheduler_run_round_robin(void);
void scheduler_compare_results(void);

#endif
