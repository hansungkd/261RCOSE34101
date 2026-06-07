#ifndef CONFIG_H
#define CONFIG_H

#include "process.h"

typedef struct {
    int process_limit;
    int arrival_time_max;
    int cpu_burst_max;
    int io_event_limit;
    int io_duration_max;
    int priority_max;
} SimulatorConfig;

const SimulatorConfig *config_get(void);
void config_print(void);
int config_set_process_limit(int value);
int config_set_arrival_time_max(int value);
int config_set_cpu_burst_max(int value);
int config_set_io_event_limit(int value);
int config_set_io_duration_max(int value);
int config_set_priority_max(int value);

#endif
