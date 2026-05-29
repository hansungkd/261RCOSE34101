#include <stdio.h>
#include <stdlib.h>

#include "process.h"

typedef enum {
    MENU_CREATE_PROCESSES = 1,
    MENU_PRINT_PROCESSES,
    MENU_RUN_FCFS,
    MENU_RUN_NONPREEMPTIVE_SJF,
    MENU_RUN_PREEMPTIVE_SJF,
    MENU_RUN_NONPREEMPTIVE_PRIORITY,
    MENU_RUN_PREEMPTIVE_PRIORITY,
    MENU_RUN_ROUND_ROBIN,
    MENU_COMPARE_RESULTS,
    MENU_EXIT = 0
} MenuOption;

static void print_menu(void)
{
    printf("\n");
    printf("CPU Scheduling Simulator\n");
    printf("========================\n");
    printf("1. Create random process set\n");
    printf("2. Print current process set\n");
    printf("3. Run FCFS scheduling\n");
    printf("4. Run Non-Preemptive SJF scheduling\n");
    printf("5. Run Preemptive SJF scheduling\n");
    printf("6. Run Non-Preemptive Priority scheduling\n");
    printf("7. Run Preemptive Priority scheduling\n");
    printf("8. Run Round Robin scheduling\n");
    printf("9. Compare scheduling results\n");
    printf("0. Exit\n");
    printf("Select: ");
}

static void clear_input_buffer(void)
{
    int ch;

    while ((ch = getchar()) != '\n' && ch != EOF) {
        /* discard remaining input */
    }
}

static int read_menu_option(void)
{
    int choice;
    int result;

    result = scanf("%d", &choice);
    if (result == EOF) {
        return MENU_EXIT;
    }

    if (result != 1) {
        printf("Invalid input. Please enter a number.\n");
        clear_input_buffer();
        return -1;
    }

    if (choice < MENU_EXIT || choice > MENU_COMPARE_RESULTS) {
        return -1;
    }

    return choice;
}

static void print_todo(const char *feature)
{
    printf("\n[%s]\n", feature);
    printf("This feature will be implemented in a later commit.\n");
}

static void handle_menu_option(int option)
{
    switch (option) {
    case MENU_CREATE_PROCESSES:
        print_todo("Create random process set");
        break;
    case MENU_PRINT_PROCESSES:
        process_print_all();
        break;
    case MENU_RUN_FCFS:
        print_todo("Run FCFS scheduling");
        break;
    case MENU_RUN_NONPREEMPTIVE_SJF:
        print_todo("Run Non-Preemptive SJF scheduling");
        break;
    case MENU_RUN_PREEMPTIVE_SJF:
        print_todo("Run Preemptive SJF scheduling");
        break;
    case MENU_RUN_NONPREEMPTIVE_PRIORITY:
        print_todo("Run Non-Preemptive Priority scheduling");
        break;
    case MENU_RUN_PREEMPTIVE_PRIORITY:
        print_todo("Run Preemptive Priority scheduling");
        break;
    case MENU_RUN_ROUND_ROBIN:
        print_todo("Run Round Robin scheduling");
        break;
    case MENU_COMPARE_RESULTS:
        print_todo("Compare scheduling results");
        break;
    case MENU_EXIT:
        printf("Bye.\n");
        break;
    default:
        printf("Unknown menu option. Please try again.\n");
        break;
    }
}

int main(void)
{
    int option;

    do {
        print_menu();
        option = read_menu_option();
        handle_menu_option(option);
    } while (option != MENU_EXIT);

    return EXIT_SUCCESS;
}
