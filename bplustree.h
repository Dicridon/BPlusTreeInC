#ifndef __FROST_BPTREE__
#define __FROST_BPTREE__

#include "list.h"
#include <stdio.h>


#define DEGREE (5)
#define MIN_ENTRIES (DEGREE)

enum bpt_node_type {
    NON_LEAF,
    LEAF
};


// Each node may have at most DEGREE children and DEGREE - 1 keys
// keys and children are paired
typedef struct bpt_node {
    union {
        struct {
            // put link at the beginning of the structure
            // so we can traverse all the leaves
            list_node_t link;
            char *data[DEGREE];
        };
        struct {
            unsigned long long num_of_children;
            // keys and children are paired during insertion
            struct bpt_node *children[DEGREE + 1];
        };
    };
    enum bpt_node_type type;
    unsigned long long num_of_keys;
    // one extra key will reudce overhead during insertion
    char *keys[DEGREE];
    struct bpt_node *parent;
} bpt_node_t;

typedef struct bpt {
    int level;
    bpt_node_t *root;
    // linked children
    list_t *list;   
    // key and data to be freed, so I may avoid accessing freed memory
    char *free_key;
    unsigned long long nodes;
} bpt_t;

bpt_t
*bpt_new();

int
bpt_insert(bpt_t *t, const char *key, const char *value);

int
bpt_delete(bpt_t *t, const char *key);

int
bpt_get(const bpt_t *t, const char *key, char *buffer);

int
bpt_range(const bpt_t *t, const char *start, const char *end, char **buffer);

int
bpt_destroy(bpt_t *t);

int
bpt_serialize_f(const bpt_t *t, const char *file_name);

int
bpt_serialize_fp(const bpt_t *t, const FILE *fp);

void
bpt_print(const bpt_t *t );

void
bpt_print_leaves(const bpt_t *t);

int
bpt_range_test(const bpt_t *t, const char *start, long n);
#endif
