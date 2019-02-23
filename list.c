#include "list.h"
list_node_t *new_list_node() {
    list_node_t *node = (list_node_t *)malloc(sizeof(list_node_t));
    node->next = node;
    node->prev = node;
    return node; // NULL is OK.
}

list_t *new_list() {
    list_t *list = (list_t *)malloc(sizeof(list_t));
    list->head = (list_node_t *)malloc(sizeof(list_node_t));
    list->head->next = list->head;
    list->head->prev = list->head;
    return list;
}

void list_add_to_head(list_t *list, list_node_t *node) {
    node->next = list->head->next;
    node->prev = list->head;
    list->head->next = node;
}

void list_add(list_node_t *ex, list_node_t *node) {
    node->next = ex->next;
    node->prev = ex;
    ex->next->prev = node;
    ex->next = node;
}

void list_remove(list_node_t *node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

void free_list_node(list_node_t *n) {
    free(n);
}

void list_destroy(list_t *t) {
    list_node_t *p = t->head->next;
    while(p != t->head) {
        t->head->next = p->next;
        free(p);
        p = t->head->next;
    }
    free(t->head);
    free(t);
    t = NULL;
}
