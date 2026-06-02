#include "priority_queue.h"

/* Swap two stored process indexes. */
static void swap_items(int *left, int *right)
{
    int temp;

    temp = *left;
    *left = *right;
    *right = temp;
}

/* Ask the scheduler-specific rule which heap item should come first. */
static int item_has_priority(const PriorityQueue *queue,
                             int candidate_pos,
                             int current_pos)
{
    int candidate_index;
    int current_index;

    candidate_index = queue->items[candidate_pos];
    current_index = queue->items[current_pos];

    return queue->has_priority(candidate_index,
                               current_index,
                               queue->context);
}

/* Move a newly inserted item upward until the heap rule is restored. */
static void bubble_up(PriorityQueue *queue, int child)
{
    while (child > 0) {
        int parent;

        parent = (child - 1) / 2;
        if (item_has_priority(queue, child, parent) == 0) {
            return;
        }

        swap_items(&queue->items[child], &queue->items[parent]);
        child = parent;
    }
}

/* Move the root item downward after pop until the heap rule is restored. */
static void bubble_down(PriorityQueue *queue, int parent)
{
    while (parent < queue->size) {
        int left_child;
        int right_child;
        int selected;

        left_child = parent * 2 + 1;
        right_child = parent * 2 + 2;
        selected = parent;

        if (left_child < queue->size) {
            if (item_has_priority(queue, left_child, selected)) {
                selected = left_child;
            }
        }

        if (right_child < queue->size) {
            if (item_has_priority(queue, right_child, selected)) {
                selected = right_child;
            }
        }

        if (selected == parent) {
            return;
        }

        swap_items(&queue->items[parent], &queue->items[selected]);
        parent = selected;
    }
}

/* Initialize an empty priority queue with its comparison rule. */
void pq_init(PriorityQueue *queue, PqPriorityRule has_priority, void *context)
{
    queue->size = 0;
    queue->has_priority = has_priority;
    queue->context = context;
}

/* Return whether the priority queue has no items. */
int pq_empty(const PriorityQueue *queue)
{
    return queue->size == 0;
}

/* Return whether the priority queue has reached fixed capacity. */
int pq_full(const PriorityQueue *queue)
{
    return queue->size == PQ_CAPACITY;
}

/* Return the number of waiting process indexes stored in the queue. */
int pq_size(const PriorityQueue *queue)
{
    return queue->size;
}

/* Insert a process index and restore heap order. */
int pq_push(PriorityQueue *queue, int process_index)
{
    if (process_index < 0) {
        return 0;
    }

    if (process_index >= PQ_CAPACITY) {
        return 0;
    }

    if (queue->has_priority == 0) {
        return 0;
    }

    if (pq_full(queue)) {
        return 0;
    }

    queue->items[queue->size] = process_index;
    bubble_up(queue, queue->size);
    queue->size++;

    return 1;
}

/* Remove the highest-priority process index. */
int pq_pop(PriorityQueue *queue, int *process_index)
{
    if (pq_empty(queue)) {
        return 0;
    }

    *process_index = queue->items[0];
    queue->size--;

    if (queue->size > 0) {
        queue->items[0] = queue->items[queue->size];
        bubble_down(queue, 0);
    }

    return 1;
}

/* Read a stored process index without removing it. */
int pq_get(const PriorityQueue *queue, int position, int *process_index)
{
    if (position < 0) {
        return 0;
    }

    if (position >= queue->size) {
        return 0;
    }

    *process_index = queue->items[position];
    return 1;
}
