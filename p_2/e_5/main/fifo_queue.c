#include <stdio.h>

// FIFO queue implementation
#define FIFO_QUEUE_CAPACITY 10

struct fifo_queue_t
{
    int data[FIFO_QUEUE_CAPACITY];
    int size;
    int head;
    int tail;
};

// FIFO queue push operation
void fifo_queue_push(struct fifo_queue_t *queue, int item)
{
    // Check if queue is full
    if (queue->size == FIFO_QUEUE_CAPACITY)
    {
        printf("Queue is full!\n");
        return;
    }

    // Push item
    queue->data[queue->tail] = item;

    // Increment tail
    queue->tail += 1;
    queue->tail %= FIFO_QUEUE_CAPACITY;

    // Increment size
    queue->size += 1;
}

// FIFO queue pop operation
int fifo_queue_pop(struct fifo_queue_t *queue)
{
    // Check if queue is empty
    if (queue->size == 0)
    {
        printf("Queue is empty!\n");
        return -1;
    }

    // Pop item
    int item = queue->data[queue->head];

    // Increment head
    queue->head += 1;
    queue->head %= FIFO_QUEUE_CAPACITY;

    // Decrement size
    queue->size -= 1;

    // Return item
    return item;
}

// FIFO queue arithmetical mean
float fifo_queue_mean(struct fifo_queue_t *queue)
{
    // Check if queue is empty
    if (queue->size == 0)
    {
        printf("Queue is empty!\n");
        return -1;
    }

    // Calculate sum
    int sum = 0;
    int i = queue->head;
    while (i - queue->head < queue->size)
    {
        sum += queue->data[i % FIFO_QUEUE_CAPACITY];
        i++;
    }

    // Return mean
    return (float)sum / queue->size;
}