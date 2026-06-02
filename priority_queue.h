#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include "process.h"

#define PQ_CAPACITY MAX_PROCESSES

typedef int (*PqPriorityRule)(int candidate_index,
                              int current_index,
                              void *context);

typedef struct {
    int items[PQ_CAPACITY];
    int size;
    PqPriorityRule has_priority;
    void *context;
} PriorityQueue;

void pq_init(PriorityQueue *queue, PqPriorityRule has_priority, void *context);
int pq_empty(const PriorityQueue *queue);
int pq_full(const PriorityQueue *queue);
int pq_size(const PriorityQueue *queue);
int pq_push(PriorityQueue *queue, int process_index);
int pq_pop(PriorityQueue *queue, int *process_index);
int pq_get(const PriorityQueue *queue, int position, int *process_index);

#endif
