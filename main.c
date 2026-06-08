#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "process.h"
#include "scheduler.h"

#define INPUT_PATH_LENGTH 256

typedef enum {
    MAIN_CREATE_PROCESS = 1,
    MAIN_CONFIG,
    MAIN_SCHEDULE,
    MAIN_EVALUATION,
    MAIN_EXIT = 0
} MainMenuOption;

typedef enum {
    PROCESS_CREATE_RANDOM = 1,
    PROCESS_LOAD_FILE,
    PROCESS_PRINT_ALL,
    PROCESS_BACK = 0
} ProcessMenuOption;

typedef enum {
    CONFIG_PRINT = 1,
    CONFIG_SET_PROCESS_LIMIT,
    CONFIG_SET_ARRIVAL_TIME_MAX,
    CONFIG_SET_CPU_BURST_MAX,
    CONFIG_SET_IO_EVENT_LIMIT,
    CONFIG_SET_IO_DURATION_MAX,
    CONFIG_SET_PRIORITY_MAX,
    CONFIG_BACK = 0
} ConfigMenuOption;

typedef enum {
    SCHEDULE_RUN_FCFS = 1,
    SCHEDULE_RUN_NONPREEMPTIVE_SJF,
    SCHEDULE_RUN_PREEMPTIVE_SJF,
    SCHEDULE_RUN_NONPREEMPTIVE_PRIORITY,
    SCHEDULE_RUN_PREEMPTIVE_PRIORITY,
    SCHEDULE_RUN_ROUND_ROBIN,
    SCHEDULE_BACK = 0
} ScheduleMenuOption;

typedef enum {
    EVALUATION_COMPARE_RESULTS = 1,
    EVALUATION_BACK = 0
} EvaluationMenuOption;

/* Print the top-level simulator menu. */
static void print_main_menu(void)
{
    printf("\n");
    printf("CPU Scheduling Simulator\n");
    printf("========================\n");
    printf("1. Create_Process\n");
    printf("2. Config\n");
    printf("3. Schedule\n");
    printf("4. Evaluation\n");
    printf("0. Exit\n");
    printf("Select: ");
}

/* Print the process creation submenu. */
static void print_process_menu(void)
{
    printf("\n");
    printf("Create_Process\n");
    printf("==============\n");
    printf("1. Create random process set\n");
    printf("2. Load process set from input file\n");
    printf("3. Print current process set\n");
    printf("0. Back\n");
    printf("Select: ");
}

/* Print the configuration submenu. */
static void print_config_menu(void)
{
    printf("\n");
    printf("Config\n");
    printf("======\n");
    printf("1. Print current configuration\n");
    printf("2. Set process limit (1-%d)\n", MAX_PROCESSES);
    printf("3. Set max arrival time\n");
    printf("4. Set max CPU burst time\n");
    printf("5. Set max I/O event count (0-%d)\n", MAX_IO_EVENTS);
    printf("6. Set max I/O duration\n");
    printf("7. Set max priority\n");
    printf("0. Back\n");
    printf("Select: ");
}

/* Read an input file path for fixed process loading. */
static int read_input_path(char path[])
{
    int length;

    printf("Input file path: ");
    if (fgets(path, INPUT_PATH_LENGTH, stdin) == NULL) {
        printf("Invalid input file path.\n");
        return 0;
    }

    length = (int)strlen(path);
    if (length > 0) {
        if (path[length - 1] == '\n') {
            path[length - 1] = '\0';
            length--;
        }
    }

    if (length == 0) {
        printf("Input file path cannot be empty.\n");
        return 0;
    }

    return 1;
}

/* Print the scheduling algorithm submenu. */
static void print_schedule_menu(void)
{
    printf("\n");
    printf("Schedule\n");
    printf("========\n");
    printf("1. Run FCFS scheduling\n");
    printf("2. Run Non-Preemptive SJF scheduling\n");
    printf("3. Run Preemptive SJF scheduling\n");
    printf("4. Run Non-Preemptive Priority scheduling\n");
    printf("5. Run Preemptive Priority scheduling\n");
    printf("6. Run Round Robin scheduling\n");
    printf("0. Back\n");
    printf("Select: ");
}

/* Print the evaluation submenu. */
static void print_evaluation_menu(void)
{
    printf("\n");
    printf("Evaluation\n");
    printf("==========\n");
    printf("1. Compare scheduling results\n");
    printf("0. Back\n");
    printf("Select: ");
}

/* Discard the rest of the current input line after bad input. */
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

/* Read one menu number and reject values outside the given menu range. */
static int read_menu_option(int max_option)
{
    int choice;
    int result;
    int too_small;
    int too_large;

    result = scanf("%d", &choice);
    if (result == EOF) {
        return 0;
    }

    if (result != 1) {
        printf("Invalid input. Please enter a number.\n");
        clear_input_buffer();
        return -1;
    }

    clear_input_buffer();

    too_small = choice < 0;
    too_large = choice > max_option;

    if (too_small) {
        return -1;
    }

    if (too_large) {
        return -1;
    }

    return choice;
}

/* Read one integer value for a configuration field. */
static int read_config_value(const char *prompt, int *value)
{
    int result;

    printf("%s", prompt);
    result = scanf("%d", value);

    if (result != 1) {
        printf("Invalid input. Please enter a number.\n");
        clear_input_buffer();
        return 0;
    }

    clear_input_buffer();

    return 1;
}

/* Change one runtime configuration value. */
static void set_config_value(const char *prompt,
                             int (*setter)(int),
                             const char *error_message)
{
    int value;
    int read_ok;
    int set_ok;

    read_ok = read_config_value(prompt, &value);
    if (read_ok == 0) {
        return;
    }

    set_ok = setter(value);
    if (set_ok == 0) {
        printf("%s\n", error_message);
        return;
    }

    printf("Configuration updated.\n");
}

/* Handle Create_Process submenu commands. */
static void handle_process_menu(void)
{
    int option;

    do {
        print_process_menu();
        option = read_menu_option(PROCESS_PRINT_ALL);

        switch (option) {
        case PROCESS_CREATE_RANDOM:
            process_create_random();
            break;
        case PROCESS_LOAD_FILE:
        {
            char path[INPUT_PATH_LENGTH];

            if (read_input_path(path)) {
                process_load_from_file(path);
            }
            break;
        }
        case PROCESS_PRINT_ALL:
            process_print_all();
            break;
        case PROCESS_BACK:
            break;
        default:
            printf("Unknown menu option. Please try again.\n");
            break;
        }
    } while (option != PROCESS_BACK);
}

/* Handle Config submenu commands. */
static void handle_config_menu(void)
{
    int option;

    do {
        print_config_menu();
        option = read_menu_option(CONFIG_SET_PRIORITY_MAX);

        switch (option) {
        case CONFIG_PRINT:
            scheduler_print_config();
            break;
        case CONFIG_SET_PROCESS_LIMIT:
            set_config_value("Process limit: ",
                             config_set_process_limit,
                             "Process limit must be between 1 and MAX_PROCESSES.");
            break;
        case CONFIG_SET_ARRIVAL_TIME_MAX:
            set_config_value("Max arrival time: ",
                             config_set_arrival_time_max,
                             "Max arrival time must be 0 or greater.");
            break;
        case CONFIG_SET_CPU_BURST_MAX:
            set_config_value("Max CPU burst time: ",
                             config_set_cpu_burst_max,
                             "Max CPU burst time must be 1 or greater.");
            break;
        case CONFIG_SET_IO_EVENT_LIMIT:
            set_config_value("Max I/O event count: ",
                             config_set_io_event_limit,
                             "Max I/O event count must be between 0 and MAX_IO_EVENTS.");
            break;
        case CONFIG_SET_IO_DURATION_MAX:
            set_config_value("Max I/O duration: ",
                             config_set_io_duration_max,
                             "Max I/O duration must be 1 or greater.");
            break;
        case CONFIG_SET_PRIORITY_MAX:
            set_config_value("Max priority: ",
                             config_set_priority_max,
                             "Max priority must be 1 or greater.");
            break;
        case CONFIG_BACK:
            break;
        default:
            printf("Unknown menu option. Please try again.\n");
            break;
        }
    } while (option != CONFIG_BACK);
}

/* Handle Schedule submenu commands. */
static void handle_schedule_menu(void)
{
    int option;

    do {
        print_schedule_menu();
        option = read_menu_option(SCHEDULE_RUN_ROUND_ROBIN);

        switch (option) {
        case SCHEDULE_RUN_FCFS:
            scheduler_run_fcfs();
            break;
        case SCHEDULE_RUN_NONPREEMPTIVE_SJF:
            scheduler_run_nonpreemptive_sjf();
            break;
        case SCHEDULE_RUN_PREEMPTIVE_SJF:
            scheduler_run_preemptive_sjf();
            break;
        case SCHEDULE_RUN_NONPREEMPTIVE_PRIORITY:
            scheduler_run_nonpreemptive_priority();
            break;
        case SCHEDULE_RUN_PREEMPTIVE_PRIORITY:
            scheduler_run_preemptive_priority();
            break;
        case SCHEDULE_RUN_ROUND_ROBIN:
            scheduler_run_round_robin();
            break;
        case SCHEDULE_BACK:
            break;
        default:
            printf("Unknown menu option. Please try again.\n");
            break;
        }
    } while (option != SCHEDULE_BACK);
}

/* Handle Evaluation submenu commands. */
static void handle_evaluation_menu(void)
{
    int option;

    do {
        print_evaluation_menu();
        option = read_menu_option(EVALUATION_COMPARE_RESULTS);

        switch (option) {
        case EVALUATION_COMPARE_RESULTS:
            scheduler_compare_results();
            break;
        case EVALUATION_BACK:
            break;
        default:
            printf("Unknown menu option. Please try again.\n");
            break;
        }
    } while (option != EVALUATION_BACK);
}

/* Dispatch one top-level menu option to its submenu. */
static void handle_main_menu_option(int option)
{
    switch (option) {
    case MAIN_CREATE_PROCESS:
        handle_process_menu();
        break;
    case MAIN_CONFIG:
        handle_config_menu();
        break;
    case MAIN_SCHEDULE:
        handle_schedule_menu();
        break;
    case MAIN_EVALUATION:
        handle_evaluation_menu();
        break;
    case MAIN_EXIT:
        printf("Bye.\n");
        break;
    default:
        printf("Unknown menu option. Please try again.\n");
        break;
    }
}

/* Main menu loop. */
int main(void)
{
    int option;

    do {
        print_main_menu();
        option = read_menu_option(MAIN_EVALUATION);
        handle_main_menu_option(option);
    } while (option != MAIN_EXIT);

    return EXIT_SUCCESS;
}
