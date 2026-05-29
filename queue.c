#include <stdio.h>

#include "queue.h"

void queue_init(ProcessQueue *queue)
{
    queue->front = 0;
    queue->rear = 0;
    queue->size = 0;
}

int queue_is_empty(const ProcessQueue *queue)
{
    return queue->size == 0;
}

int queue_is_full(const ProcessQueue *queue)
{
    return queue->size == QUEUE_CAPACITY;
}

int queue_size(const ProcessQueue *queue)
{
    return queue->size;
}

int queue_enqueue(ProcessQueue *queue, int process_index)
{
    if (process_index < 0 || process_index >= QUEUE_CAPACITY || queue_is_full(queue)) {
        return 0;
    }

    queue->items[queue->rear] = process_index;
    queue->rear = (queue->rear + 1) % QUEUE_CAPACITY;
    queue->size++;

    return 1;
}

int queue_dequeue(ProcessQueue *queue, int *process_index)
{
    if (queue_is_empty(queue)) {
        return 0;
    }

    *process_index = queue->items[queue->front];
    queue->front = (queue->front + 1) % QUEUE_CAPACITY;
    queue->size--;

    return 1;
}

int queue_peek(const ProcessQueue *queue, int *process_index)
{
    if (queue_is_empty(queue)) {
        return 0;
    }

    *process_index = queue->items[queue->front];
    return 1;
}

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
        position = (queue->front + i) % QUEUE_CAPACITY;

        if (queue->items[position] >= 0 && queue->items[position] < QUEUE_CAPACITY) {
            printf("P%d", processes[queue->items[position]].pid);
        } else {
            printf("?");
        }

        if (i < queue->size - 1) {
            printf(" -> ");
        }
    }

    printf("\n");
}
