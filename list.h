#ifndef __FROST_LIST__
#define __FROST_LIST__
#include <stdlib.h>

typedef struct list_node {
    struct list_node *prev;
    struct list_node *next;
} list_node_t;

typedef struct list {
    list_node_t *head;
} list_t;

list_node_t *new_list_node();

list_t *new_list();

void list_add_to_head(list_t *list, list_node_t *node);

void list_add(list_node_t *ex, list_node_t *node) ;

void list_remove(list_node_t *node);

void list_destroy(list_t *t);

void free_list_node(list_node_t *l);
#endif
