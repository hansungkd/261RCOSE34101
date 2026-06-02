#include <stdio.h>

#include "queue.h"

/* Start an empty circular FIFO queue. */
void queue_init(ProcessQueue *queue)
{
    queue->front = 0;
    queue->rear = 0;
    queue->size = 0;
}

/* Return whether the queue has no process indexes. */
int queue_is_empty(const ProcessQueue *queue)
{
    return queue->size == 0;
}

/* Return whether the queue cannot accept more process indexes. */
int queue_is_full(const ProcessQueue *queue)
{
    return queue->size == QUEUE_CAPACITY;
}

/* Return the current number of process indexes. */
int queue_size(const ProcessQueue *queue)
{
    return queue->size;
}

/* Add a process index to the rear of the FIFO queue. */
int queue_enqueue(ProcessQueue *queue, int process_index)
{
    int next_rear;

    if (process_index < 0) {
        return 0;
    }

    if (process_index >= QUEUE_CAPACITY) {
        return 0;
    }

    if (queue_is_full(queue)) {
        return 0;
    }

    queue->items[queue->rear] = process_index;
    next_rear = queue->rear + 1;
    queue->rear = next_rear % QUEUE_CAPACITY;
    queue->size++;

    return 1;
}

/* Remove the oldest process index from the front of the FIFO queue. */
int queue_dequeue(ProcessQueue *queue, int *process_index)
{
    int next_front;

    if (queue_is_empty(queue)) {
        return 0;
    }

    *process_index = queue->items[queue->front];
    next_front = queue->front + 1;
    queue->front = next_front % QUEUE_CAPACITY;
    queue->size--;

    return 1;
}

/* Read the front process index without removing it. */
int queue_peek(const ProcessQueue *queue, int *process_index)
{
    if (queue_is_empty(queue)) {
        return 0;
    }

    *process_index = queue->items[queue->front];
    return 1;
}

/* Print queue contents using process IDs instead of raw array indexes. */
void queue_print(const char *name, const ProcessQueue *queue, const Process processes[])
{
    int i;
    int position;

    printf("%s: ", name);

    if (queue_is_empty(queue)) {
        printf("(empty)\n");
        return;
    }

    for (i = 0; i < queue->size; i++) {
        int item_index;
        int valid_index;

        position = (queue->front + i) % QUEUE_CAPACITY;
        item_index = queue->items[position];

        valid_index = 1;
        if (item_index < 0) {
            valid_index = 0;
        }

        if (item_index >= QUEUE_CAPACITY) {
            valid_index = 0;
        }

        if (valid_index) {
            printf("P%d", processes[item_index].pid);
        } else {
            printf("?");
        }

        if (i < queue->size - 1) {
            printf(" -> ");
        }
    }

    printf("\n");
}
