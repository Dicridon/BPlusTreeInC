#include "queue.h"
static queue_node_t *new_queue_node(void *data) {
    queue_node_t *node = malloc(sizeof(queue_node_t));

    node->next = NULL;
    node->data = data;
    return node;
}

queue_t *new_queue() {
    queue_t *q = malloc(sizeof(queue_t));
    q->head = new_queue_node(NULL);
    q->head->next = NULL;
    q->tail = q->head;
    return q;
}

bool queue_empty(queue_t *q) {
    return q->head->next == NULL;
}

void enqueue(queue_t *q, void *data) {
    queue_node_t *node = new_queue_node(data);
    q->tail->next = node;
    q->tail = node;
}

void dequeue(queue_t *q, void **data) {
    if (queue_empty(q)) {
        *data = NULL;
        return;
    }


    *data = q->head->next->data;
    queue_node_t *p = q->head->next;
    q->head->next = q->head->next->next;
    free(p);
    if (queue_empty(q))
        q->tail = q->head;
}

void queue_destroy(queue_t *q) {
    void *data = NULL;
    
    do {
        dequeue(q, &data);
    } while(data);

    free(q->head);
    free(q);
}
