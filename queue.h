#ifndef QUEUE_H
#define QUEUE_H

#include "process.h"

#define QUEUE_CAPACITY MAX_PROCESSES

typedef struct {
    int items[QUEUE_CAPACITY];
    int front;
    int rear;
    int size;
} ProcessQueue;

typedef ProcessQueue ReadyQueue;
typedef ProcessQueue WaitingQueue;

void queue_init(ProcessQueue *queue);
int queue_is_empty(const ProcessQueue *queue);
int queue_is_full(const ProcessQueue *queue);
int queue_size(const ProcessQueue *queue);
int queue_enqueue(ProcessQueue *queue, int process_index);
int queue_dequeue(ProcessQueue *queue, int *process_index);
int queue_peek(const ProcessQueue *queue, int *process_index);
void queue_print(const char *name, const ProcessQueue *queue, const Process processes[]);

#endif
