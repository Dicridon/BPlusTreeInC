#ifndef __FROST_QUEUE__
#define __FROST_QUEUE__
#include <stdlib.h>
#include <stdbool.h>
typedef struct queue_node {
    struct queue_node *next;
    void *data;
} queue_node_t;

typedef struct queue {
    queue_node_t *head;
    queue_node_t *tail;
} queue_t;

queue_t *new_queue();

bool queue_empty(queue_t *q);

void enqueue(queue_t *q, void *data);

void dequeue(queue_t *q, void **data);

void queue_destroy(queue_t *q);
#endif
