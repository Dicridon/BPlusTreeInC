/*
  !!!!!!!!!!!!!!
  !!!! NOTE: I ALWAYS ASSUME THAT MALLOC/REALLOC/CALLOC WOULD NOT FAIL!!!!!!
  !!!!       THIS IS JUST FOR TEST
  !!!!!!!!!!!!!!
*/

#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include "list.h"
#include "queue.h"
#include "bplustree.h"

// insertion
static bpt_node_t *bpt_new_leaf();
static bpt_node_t *bpt_new_non_leaf();
static void bpt_insert_child(bpt_node_t *old, bpt_node_t * new);
static int bpt_complex_insert(bpt_t *t, bpt_node_t *leaf,
                              char *key, char *value);
static int bpt_simple_insert(bpt_node_t *leaf, char *key, char *value);
static int bpt_insert_adjust(bpt_t * t, bpt_node_t *old, bpt_node_t *new);
static bpt_node_t *find_leaf(bpt_t *t, char *key);
static bpt_node_t *find_non_leaf(bpt_t *t, char *key);

// deletion
static bool bpt_is_root(bpt_node_t *t);
static int bpt_insert_key(bpt_node_t *t, char *key);
static bpt_node_t* bpt_check_redistribute(bpt_node_t *t);
static int redistribute_leaf(bpt_t *t, bpt_node_t *leaf, char *key);
static int redistribute_internal(char *split_key, bpt_node_t *parent,
                                 bpt_node_t *left, bpt_node_t *right);
static int merge_leaves(bpt_t *t, bpt_node_t *leaf, char *key);
static int merge(bpt_t *t, bpt_node_t *parent, char *key, char *split_key);
static int bpt_remove_key_and_data(bpt_node_t *node, char *key);
static int bpt_complex_delete(bpt_t *t, bpt_node_t *leaf, char *key);
static int bpt_simple_delete(bpt_t *t, bpt_node_t *leaf, char *key);
static void bpt_free_leaf(bpt_node_t *leaf);
static void bpt_free_non_leaf(bpt_node_t *nleaf);

bpt_t *bpt_new();
int bpt_insert(bpt_t *t, char *key, char *value);
int bpt_delete(bpt_t *t, char *key);
int bpt_get(bpt_t *t, char *key, char *buffer);
int bpt_range(bpt_t *t, char *start, char *end, char **buffer);
int bpt_destroy(bpt_t *t);
int bpt_serialize_f(bpt_t *t, char *file_name);
int bpt_serialize_fp(bpt_t *t, FILE *fp);
void bpt_print(bpt_t * t);
void bpt_print_leaves(bpt_t *t);

bpt_t *bpt_new() {
    bpt_t *bpt = malloc(sizeof(bpt_t));
    bpt->root = bpt_new_leaf();
    
    bpt->list = new_list();
    list_add(bpt->list->head, &bpt->root->link);
    
    bpt->nodes = 0;
    bpt->level = 1;
    return bpt;

}

static bpt_node_t *bpt_new_leaf() {
    bpt_node_t *node = malloc(sizeof(bpt_node_t));
    //    node->link = new_list_node();
    node->parent = NULL;
    node->type = LEAF;
    for (int i = 0; i < DEGREE; i++) {
        node->keys[i] = NULL;
        node->data[i] = NULL;        
    }
    return node;
}

static bpt_node_t *bpt_new_non_leaf() {
    bpt_node_t *node = malloc(sizeof(bpt_node_t));
    node->parent = NULL;
    node->type = NON_LEAF;
    
    for (int i = 0; i < DEGREE; i++) {
        node->keys[i] = NULL;
        node->children[i] = NULL;        
    }
    node->children[DEGREE] = NULL;
    
    return node;
}

// outter function has determined that bpt_complex_insert should be called
// we have to split the leaf and insert new node into parent node
// adjustment to parent is necessary
static int bpt_simple_insert(bpt_node_t *leaf, char *key, char *value) {
    unsigned long long i;
    for (i = 0; i < leaf->num_of_keys; i++) {
        // if the tree is empty, we may encounter a NULL key
        if (leaf->keys[i] == NULL || strcmp(leaf->keys[i], key) >= 0)
            break;
    }

    for (unsigned long long j = leaf->num_of_keys; j > i; j--) {
        leaf->keys[j] = leaf->keys[j - 1];
        leaf->data[j] = leaf->data[j - 1];
    }
    leaf->keys[i] = malloc(strlen(key));
    leaf->data[i] = malloc(strlen(value));
    strcpy(leaf->keys[i], key);
    strcpy(leaf->data[i], value);
    leaf->num_of_keys++; 
    return 1;
}

static void bpt_insert_child(bpt_node_t *old, bpt_node_t * new) {
    bpt_node_t *parent = old->parent;
    // insert
    unsigned long long i = 0;
    unsigned long long j = 0;
    // new inherits data from old, so we just need to insert new after old
    for (i = 0; i < parent->num_of_children; i++) {
        if (parent->children[i] == old)
            break;
    }

    for (j = parent->num_of_keys; j > i; j--) {
        parent->keys[j] = parent->keys[j-1];
    }
    parent->keys[i] = new->keys[0];
    parent->num_of_keys++;

    i++; // insert after old
    for (j = parent->num_of_children; j > i; j--) {
        parent->children[j] = parent->children[j-1];
    }
    parent->children[i] = new;
    parent->num_of_children++;
    new->parent = parent;
 }

static int bpt_insert_adjust(bpt_t *t, bpt_node_t *old, bpt_node_t *new) {
    bpt_node_t *parent = old->parent;
    if (t->level == 1) {
        bpt_node_t *new_parent = bpt_new_non_leaf();
        new_parent->num_of_children = 2;
        new_parent->num_of_keys = 1;
        new_parent->keys[0] = new->keys[0];
        new_parent->children[0] = old;
        new_parent->children[1] = new;
        t->root = new_parent;
        old->parent = new_parent;
        new->parent = new_parent;

        t->level++;
        return 1;        
    }
    
    if (!parent) {
        // this is internal node split
        // we may not just create a new root and link old and new to it
        // we must pick out the smallest child in new and use this child
        // to construct a new root, or we may not ensure the relationsip
        // that num_of_childen = num_of_keys + 1
        bpt_node_t *new_parent = bpt_new_non_leaf();
        new_parent->num_of_children = 2;
        new_parent->num_of_keys = 1;
        new_parent->keys[0] = new->keys[0];
        new_parent->children[0] = old;
        new_parent->children[1] = new;
        t->root = new_parent;
        t->level++;

        // delete the key copied to root in new
        for (unsigned long long i = 1; i < new->num_of_keys; i++) {
            new->keys[i-1] = new->keys[i];
        }
        new->keys[new->num_of_keys - 1] = NULL;
        new->num_of_keys--;

        old->parent = new_parent;
        new->parent = new_parent;
        // num_of_children will not be modified
        // we modify this field in else if
        
    } else if (parent->num_of_keys == DEGREE - 1) {
        // insert
        // new will be added to parent of old
        bpt_insert_child(old, new); 
        if (new->type == NON_LEAF) {
            // delete the key copied to parent in new
            for (unsigned long long i = 1; i < new->num_of_keys; i++) {
                new->keys[i-1] = new->keys[i];
            }
            new->keys[new->num_of_keys - 1] = NULL;
            new->num_of_keys--;
        }
        
        // split
        // [split] is NOT left to new parent
        bpt_node_t * new_parent = bpt_new_non_leaf();
        unsigned long long split = parent->num_of_keys / 2;
        unsigned long long i = 0;
        for (i = 0; i + split < parent->num_of_keys; i++) {
            new_parent->keys[i] = parent->keys[i + split];
            parent->keys[i + split] = NULL;
            new_parent->children[i] = parent->children[i + split + 1];
            new_parent->children[i ]->parent = new_parent;
            parent->children[i + split + 1] = NULL;
        }

        new_parent->num_of_keys = parent->num_of_keys - split;
        new_parent->num_of_children = new_parent->num_of_keys;
        parent->num_of_keys -= parent->num_of_keys - split;
        parent->num_of_children = parent->num_of_keys + 1;

        
        // recursive handling
        bpt_insert_adjust(t, parent, new_parent);
    } else {
        bpt_insert_child(old, new);
        if (new->type == NON_LEAF) {
            // delete the key copied to parent in new
            for (unsigned long long i = 1; i < new->num_of_keys; i++) {
                new->keys[i-1] = new->keys[i];
            }
            new->keys[new->num_of_keys - 1] = NULL;
            new->num_of_keys--;
        }
    }
    return 1;
}


// outter function has determined to call this function
// so leaf->num_of_keys = DEGREE - 1
static int bpt_complex_insert(bpt_t *t, bpt_node_t *leaf, char *key,
                              char *value) {
    bpt_node_t *new_leaf = bpt_new_leaf();
    // since we have make extra space
    // we may just insert the key and vlaue into old leaf and then split it
    // what good about this is that we do not have to consider where
    // new data should be inserted.
    unsigned long long i = 0;
    for (i = 0; i < leaf->num_of_keys; i++) {
        if(strcmp(leaf->keys[i], key) >= 0)
            break;
    }

    for (unsigned long long j = leaf->num_of_keys; j > i; j--) {
        leaf->keys[j] = leaf->keys[j - 1];
        leaf->data[j] = leaf->data[j - 1];
    }

    leaf->keys[i] = malloc((strlen(key)) + 1);
    leaf->data[i] = malloc((strlen(value)) + 1);

    strcpy(leaf->keys[i], key);
    strcpy(leaf->data[i], value);

    // now split this leaf
    // copy the right half to new leaf (including [split])
    unsigned long long split = leaf->num_of_keys / 2;

    // notice the <= sign here!! DO NOT forget the extra space
    for (i = 0; i + split <= leaf->num_of_keys; i++) {
        new_leaf->keys[i] = leaf->keys[i + split];
        new_leaf->data[i] = leaf->data[i + split];
        leaf->keys[i + split] = NULL;
        leaf->data[i + split] = NULL;
    }

    // we ensure that new leaf is always on the right of leaf
    new_leaf->num_of_keys = i;
    leaf->num_of_keys -= leaf->num_of_keys - split;
    
    list_add(&leaf->link, &new_leaf->link);
    bpt_insert_adjust(t, leaf, new_leaf);

    return 1;
}

int bpt_get(bpt_t *t, char *key, char *buffer) {
    bpt_node_t *walk = t->root;
    unsigned long long i = 0;
    while(walk->type != LEAF) {
        for (i = 0; i < walk->num_of_keys; i++) {
            if (strcmp(walk->keys[i], key) > 0)
                break;
        }
        walk = walk->children[i];
    }

    for (i = 0; i < walk->num_of_keys; i++) {
        if (strcmp(walk->keys[i], key) == 0) {
            if (buffer != NULL) {
                strcpy(buffer, walk->data[i]);
            }
            return strlen(walk->data[i]);
        }
    }
    return -1;
}

int bpt_range(bpt_t *t, char *start, char *end, char **buffer) {
    if (strcmp(start, end) > 0)
        return -1;
    
    bpt_node_t *leaf = find_leaf(t, start);
    unsigned long long i = 0, j = 0;

    list_node_t *p = &leaf->link;
    bpt_node_t *node;
    while(p != t->list->head) {
        node = (bpt_node_t *)p;
        for (i = 0; i < node->num_of_keys; i++) {
            if (strcmp(node->keys[i], end) > 0)
                return 1;

            // if (strcmp(node->keys[i], start) >= 0)
            //     printf("%s, ", node->data[i]);

            if (strcmp(node->keys[i], start) >= 0)
                strcpy(buffer[j++], node->keys[i]);
        }
        p = p->next;
    }
    return -1;
}

int bpt_destroy(bpt_t *t) {
    if (!t->root) {
        printf("empty tree\n");
        return 1;
    }
    bpt_node_t __walk__;
    bpt_node_t *__walk = &__walk__;
    bpt_node_t **walk = &__walk;
    
    queue_t *queue = new_queue();
    enqueue(queue, t->root);

    while(!queue_empty(queue)) {
        dequeue(queue, (void*)walk);
        for (unsigned long long i = 0;
             (*walk)->type == NON_LEAF && i < (*walk)->num_of_children; i++) {
            enqueue(queue, (*walk)->children[i]);
        }

        if ((*walk)->type == LEAF) {
            for (unsigned long long i = 0; i < (*walk)->num_of_keys; i++) { 
                free((*walk)->keys[i]);
                free((*walk)->data[i]);
            }
        }
        free((*walk));
    }
    queue_destroy(queue);
    return 1;
}

// find leaf will find a leaf suitable for this key
static bpt_node_t *find_leaf(bpt_t * t, char *key) {
    bpt_node_t *walk = t->root;
    unsigned long long i = 0;
    while(walk->type != LEAF) {
        for (i = 0; i < walk->num_of_keys; i++) {
            if (strcmp(walk->keys[i], key) > 0)
                break;
        }
        walk = walk->children[i];
    }
    return walk;
}

static bpt_node_t *find_non_leaf(bpt_t *t, char *key) {
    bpt_node_t *walk = t->root;
    unsigned long long i = 0;
    while(walk->type != LEAF) {
        for (i = 0; i < walk->num_of_keys; i++) {
            if (strcmp(walk->keys[i], key) == 0)
                return walk;
            if (strcmp(walk->keys[i], key) > 0)
                break;
        }
        walk = walk->children[i];
    }
    return NULL;
}

int bpt_insert(bpt_t *t, char *key, char *value) {
    bpt_node_t *old = find_leaf(t, key);

    for (unsigned long long i = 0; i < old->num_of_keys; i++) {
        if (strcmp(old->keys[i], key) == 0) {
            old = NULL;
            break;
        }
    }

    if (!old)
        return 1;
    
    t->nodes++;
    
    if (old->num_of_keys == DEGREE - 1) {
        // if you want to figure out what is actually going on
        // chekc out website below
        // https://www.cs.usfca.edu/~galles/visualization/BPlustree.html
        // this page indeed benefited me so much
        bpt_complex_insert(t, old, key, value);
    } else {
        bpt_simple_insert(old, key, value);
    }
    return 1;
}

// ensure leaf has no data at all before calling this function
static void bpt_free_leaf(bpt_node_t *leaf) {
    free(leaf);
}

static void bpt_free_non_leaf(bpt_node_t *nleaf) {
    free(nleaf);
}

void bpt_print(bpt_t * t) {
    if (!t->root) {
        printf("empty tree\n");
        return;
    }
    bpt_node_t __walk__;
    bpt_node_t *__walk = &__walk__;
    bpt_node_t **walk = &__walk;
    
    queue_t *queue = new_queue();
    enqueue(queue, t->root);

    while(!queue_empty(queue)) {
        dequeue(queue, (void*)walk);
        for (unsigned long long i = 0;
             (*walk)->type == NON_LEAF && i < (*walk)->num_of_children; i++) {
            enqueue(queue, (*walk)->children[i]);
        }
        printf("%p child of %p: \n",*(walk), (*walk)->parent);
        if ((*walk)->type == NON_LEAF)
            printf("non leaf: %llu children, %llu keys\n",
                   (*walk)->num_of_children, (*walk)->num_of_keys);
        printf("keys: \n");
        for (unsigned long long i = 0; i < (*walk)->num_of_keys; i++) {
            printf("%s, ", (*walk)->keys[i]);
        }
        printf("\n\n");
    }
    queue_destroy(queue);
}

void bpt_print_leaves(bpt_t *t) {
    list_node_t *p = t->list->head->next;
    bpt_node_t *node;
    while(p != t->list->head) {
        node = (bpt_node_t *)p;
        for (unsigned long long i = 0; i < node->num_of_keys; i++) {
            printf("%s, ", node->keys[i]);
        }
        p = p->next;
    }
    puts("");
}

// deletion

/*
  - How to delete
  1. if leaf->num_of_keys > DEGREE / 2, just delete the key
  2. if leaf->num_of_keys = DEGREE / 2, try to borrow a key from sibling
  3. if siblings can not offer a key, merge this leaf and the sibling which has
     only DEGREE keys, the recursively remove
*/

static bool bpt_is_root(bpt_node_t * t) {
    return t->parent == NULL;
}

static int bpt_remove_key_and_data(bpt_node_t *node, char *key) {
    unsigned long long i = 0;
    for (i = 0; i < node->num_of_keys; i++) {
        if (strcmp(node->keys[i], key) == 0)
            break;
    }

    for(; i < node->num_of_keys; i++) {
        node->keys[i] = node->keys[i + 1];
        node->data[i] = node->data[i + 1];
    }

    node->num_of_keys--;
    free(node->keys[node->num_of_keys]);
    free(node->data[node->num_of_keys]);
    node->keys[node->num_of_keys] = NULL;
    node->data[node->num_of_keys] = NULL;
    return 1;
}

static bpt_node_t* bpt_check_redistribute(bpt_node_t *t) {
    if (!t->parent)
        return NULL;

    bpt_node_t *parent = t->parent;
    unsigned long long i = 0;
    for (i = 0; i < parent->num_of_children; i++) {
        if (parent->children[i] == t)
            break;
    }

    bpt_node_t *left = parent->children[(i == 0) ? 0 : i - 1];
    bpt_node_t *right =
        parent->children[(i == parent->num_of_children - 1) ? i : i + 1];

    if (left->num_of_keys > DEGREE / 2)
        return left;
    else if (right->num_of_keys > DEGREE / 2) {
        return right;
    }
    return NULL;
}


// redistribute leaf nodes, not internal nodes
static int redistribute_leaf(bpt_t *t, bpt_node_t *leaf, char *key) {
    bpt_node_t *parent = leaf->parent;
    bpt_node_t *non_leaf = find_non_leaf(t, key);
    unsigned long long i = 0;
    unsigned long long idx_replace_key = 0;
    char *replace_key = NULL;

    if (non_leaf) {
        for (idx_replace_key = 0;
             idx_replace_key < non_leaf->num_of_keys; idx_replace_key++)
        if (strcmp(non_leaf->keys[idx_replace_key], key) == 0)
            break;            
    }

    
    for (i = 0; i < parent->num_of_children; i++)
        if (parent->children[i] == leaf)
            break;

    unsigned long long idx_left = (i == 0) ? 0 : i - 1;
    unsigned long long idx_right =
        (i == parent->num_of_children - 1) ? i : i + 1;
    bpt_node_t *left = parent->children[idx_left];
    bpt_node_t *right = parent->children[idx_right];
 
    if (left->num_of_keys > DEGREE / 2) {
        unsigned long long tail = left->num_of_keys - 1;
        parent->keys[idx_left] = left->keys[tail];
        
        for (i = 0; i < leaf->num_of_keys; i++) {
            if (strcmp(leaf->keys[i], key) == 0) {
                free(leaf->keys[i]);
                free(leaf->data[i]);
                for (; i > 0; i--) {
                    leaf->keys[i] = leaf->keys[i-1];
                    leaf->data[i] = leaf->data[i-1];
                }
                leaf->keys[0] = left->keys[tail];
                leaf->data[0] = left->data[tail];
                left->keys[tail] = NULL;
                left->data[tail] = NULL;
                left->num_of_keys--;
            }
            replace_key = leaf->keys[0];
            break;
        }
    }
    else if (right->num_of_keys > DEGREE / 2) {
        unsigned long long tail = leaf->num_of_keys - 1;
        for (i = 0; i < leaf->num_of_keys; i++) {
            if (strcmp(leaf->keys[i], key) == 0) {
                free(leaf->keys[i]);
                free(leaf->data[i]);
                for (; i < leaf->num_of_keys - 1; i++) {
                    leaf->keys[i] = leaf->keys[i+1];
                    leaf->data[i] = leaf->data[i+1];
                }
                leaf->keys[tail] = right->keys[0];
                leaf->data[tail] = right->data[0];

                for (unsigned long long j = 0; j < right->num_of_keys; j++) {
                    right->keys[j] = right->keys[j+1];
                    right->data[j] = right->data[j+1];
                }
                parent->keys[i] = right->keys[0];
                right->num_of_keys--;
            }
            replace_key =  leaf->keys[0];
            break;
        }
    }

    // if the key deleted reside in grandparent's keys, replace it
    if (non_leaf)
        non_leaf->keys[idx_replace_key] = replace_key;

    return 1;
}

static int bpt_simple_delete(bpt_t *t, bpt_node_t *leaf, char *key) {

    if (!bpt_is_root(leaf)) {
        // this branch will be executed only when deletin takes place on a
        // right subtree of a key
        bpt_node_t *non_leaf = find_non_leaf(t, key);
        unsigned long long i = 0;
        if (non_leaf) {
            for (i = 0; i < non_leaf->num_of_keys; i++)
                if (strcmp(non_leaf->keys[i], key) == 0) {
                    break;
                }
        }
        bpt_remove_key_and_data(leaf, key);
        // replace the key
        if (non_leaf)
            non_leaf->keys[i] = leaf->keys[0];
        return 1;
    } else
        return bpt_remove_key_and_data(leaf, key);
}



// only used to merge leaves
static int merge_leaves(bpt_t *t, bpt_node_t *leaf, char *key) {
    bpt_node_t *parent = leaf->parent;
    unsigned long long i;
    unsigned long long idx_left;
    unsigned long long idx_right;
    // delete the key
    for (i = 0; i < leaf->num_of_keys; i++) {
        if (strcmp(leaf->keys[i], key) == 0)
            break;
    }

    t->free_key = leaf->keys[i];
    free(leaf->data[i]);
    
    for (unsigned long long j = i; j <= leaf->num_of_keys - 1; j++) {
        leaf->keys[j] = leaf->keys[j+1];
        leaf->data[j] = leaf->data[j+1];
    }
    leaf->num_of_keys--;

    // find a proper sibling
    for (i = 0; i < parent->num_of_children; i++) {
        if (parent->children[i] == leaf)
            break;
    }

    idx_left = (i == 0) ? 0 : i - 1;
    idx_right = (i == parent->num_of_children - 1) ? i : i + 1;
    bpt_node_t *left = parent->children[idx_left];
    bpt_node_t *right = parent->children[idx_right];
    unsigned long long delete_position = 0;

    // merge to right
    // merge left
    if (i == parent->num_of_children - 1 || right->num_of_keys > DEGREE / 2) {
        delete_position = i;
        right = NULL;
    } else {
        delete_position = idx_right;
        left = NULL;        
    }

    // merge
    if (left) {
        // merge leaf into its left sibling
        for (i = 0; i < leaf->num_of_keys; i++) {
            left->keys[i + left->num_of_keys] = leaf->keys[i];
            leaf->keys[i] = NULL;
            left->data[i + left->num_of_keys] = leaf->data[i];
            leaf->data[i] = NULL;
        }
        leaf->link.prev->next = leaf->link.next;
        leaf->link.next->prev = leaf->link.prev;
        left->num_of_keys += leaf->num_of_keys;
        bpt_free_leaf(leaf);
    } else {
        // merge leaf into its right sibling
        // we merge right into leaf and modify the point in parent pointing
        // to right to point to leaf, so we may reduce some overhead
        for (i = 0; i < right->num_of_keys; i++) {
            leaf->keys[i + leaf->num_of_keys] = right->keys[i];
            right->keys[i] = NULL;
            leaf->data[i + leaf->num_of_keys] = right->data[i];
            right->data[i] = NULL;

        }
        right->link.prev->next = right->link.next;
        right->link.next->prev =right->link.prev;
        parent->children[idx_right] = leaf;
        leaf->num_of_keys += right->num_of_keys;
        bpt_free_leaf(right);
    }
    
    // align children
    for (unsigned long long j = delete_position; j <= parent->num_of_children - 1; j++) {
        parent->children[j] = parent->children[j+1];
    }
    parent->num_of_children--;
    return 1;
}


static int bpt_insert_key(bpt_node_t *t, char *key) {
    unsigned long long i = 0;
    for (i = 0; i < t->num_of_keys; i++) {
        if (strcmp(t->keys[i], key) == 0)
            return 1;

        if (strcmp(t->keys[i], key) > 0)
            break;
    }

    for (unsigned long long j = t->num_of_keys; j > i; j--) {
        t->keys[j] = t->keys[j-1];
    }
    t->keys[i] = key;
    t->num_of_keys++;
    return 1;
}

static int redistribute_internal(char *split_key, bpt_node_t *node,
                                 bpt_node_t *left, bpt_node_t *right) {
    bpt_node_t *parent = node->parent;
    unsigned long long i = 0;
    unsigned long long j = 0;
    unsigned long long split = 0;
    for (split = 0; split < parent->num_of_keys; split++) {
        if (strcmp(parent->keys[split], split_key) == 0)
            break;
    }

    for (i = 0; i < parent->num_of_children; i++) {
        if (parent->children[i] == node)
            break;
    }
    
    if (!left && right->num_of_keys > DEGREE / 2) {
        node->keys[node->num_of_keys++] = split_key;    
        node->children[node->num_of_children++] = right->children[0];
        right->children[0]->parent = node;
        parent->keys[split] = right->keys[0];
        for (j = 0; j <= right->num_of_keys - 1; j++) {
            right->keys[j] = right->keys[j+1];
            right->children[j] = right->children[j+1];
        }
        right->children[j] = NULL;
        right->num_of_children--;
        right->num_of_keys--;
    } else if (!right && left->num_of_keys > DEGREE / 2) {
        // make some space
        node->children[node->num_of_children] =
            node->children[node->num_of_children-1];
        for (j = node->num_of_keys; j > 0; j--) {
            node->keys[j] = node->keys[j-1];
            node->children[j] = node->children[j-1];
        }
        node->keys[0] = split_key;
        node->children[0] = left->children[left->num_of_children-1];
        left->children[left->num_of_children-1]->parent = node;
        parent->keys[split] = left->keys[left->num_of_keys-1];
        left->keys[left->num_of_keys-1] = NULL;
        left->children[left->num_of_children-1] = NULL;
        node->num_of_keys++;
        node->num_of_children++;
        left->num_of_children--;
        left->num_of_keys--;
    } else if (left->num_of_keys > DEGREE/2 && right->num_of_keys > DEGREE/2) {
        // borrow from right
        node->children[node->num_of_children++] = right->children[0];
        right->children[0]->parent = node;
 node->keys[node->num_of_keys++] = split_key;
        parent->keys[split] = right->keys[0];
        for (j = 0; j <= right->num_of_keys - 1; j++) {
            right->keys[j] = right->keys[j+1];
            right->children[j] = right->children[j+1];
        }
        right->children[j+1] = NULL;
        right->num_of_children--;
        right->num_of_keys--;
    } else {
        return -1;
    }
    return 1;
}

static int merge_internal(bpt_t *t, bpt_node_t *parent, char *split_key) {
    // merge parent into a proper sibling, incorporating a split key from parent
    // of parent which seperate this parent and the sibling
    unsigned long long i = 0;
    bpt_node_t *grandparent = parent->parent;


    // merge this parent and its sibling
    unsigned long long idx_left = 0;
    unsigned long long idx_right = 0;
    for (i = 0; i < grandparent->num_of_children; i++) {
        if (grandparent->children[i] == parent)
            break;
    }

    idx_left = (i == 0) ? 0 : i - 1;
    idx_right = (i == grandparent->num_of_children - 1) ? i : i + 1;
    bpt_node_t *left = grandparent->children[idx_left];
    bpt_node_t *right = grandparent->children[idx_right];
    bool left_available = false;
    bool right_available = false;
    unsigned long long delete_position = i;
    if (i == 0 || left->num_of_keys > DEGREE / 2) {
        delete_position = idx_right;
        if (left->num_of_keys > DEGREE / 2)
            left_available = true;
        left = NULL;
    }

    if (i == grandparent->num_of_children-1 || right->num_of_keys > DEGREE/2) {
        delete_position = i;
        if (right->num_of_keys > DEGREE / 2)
            right_available = true;
        right = NULL;
    }

    // what sucks is that an internal node may have no proper sibling to merge in
    // so what should we do ?
    // we have to borrow a key from proper sibling just as what we do to leaves.
    if (!left && !right) {  // merge is impossible
        if (left_available)
            redistribute_internal(split_key, parent,
                                  grandparent->children[idx_left], NULL);
        else
            redistribute_internal(split_key, parent, NULL,
                                  grandparent->children[idx_right]);
        return 1;
    }

    
    if (left) {
        for (i = 0; i < parent->num_of_keys; i++) {
            left->keys[i+left->num_of_keys] = parent->keys[i];
            parent->keys[i] = NULL;
            left->children[i+left->num_of_children] = parent->children[i];
            parent->children[i]->parent = left;
            parent->children[i] = NULL;
        }
        
        left->children[i+left->num_of_children] = parent->children[i];
        parent->children[i]->parent = left;
        parent->children[i] = NULL;

        left->num_of_keys += parent->num_of_keys;
        left->num_of_children += parent->num_of_children;
        
        bpt_free_non_leaf(parent);
        bpt_insert_key(left, split_key);
    } else {
        for (i = 0; i < right->num_of_keys; i++) {
            parent->keys[i+parent->num_of_keys] = right->keys[i];
            right->keys[i] = NULL;
            parent->children[i+parent->num_of_children] = right->children[i];
            right->children[i]->parent = parent;
            right->children[i] = NULL;
        }
        
        parent->children[i+parent->num_of_children] = right->children[i];
        right->children[i]->parent = parent;
        right->children[i] = NULL;

        parent->num_of_keys += right->num_of_keys;
        parent->num_of_children += right->num_of_children;

        bpt_free_non_leaf(right);
        grandparent->children[idx_right] = parent;
        bpt_insert_key(parent, split_key);            
    }

    // align children
    for (unsigned long long j = delete_position;
         j <= grandparent->num_of_children - 1;
         j++) {
        grandparent->children[j] = grandparent->children[j+1];
    }
    grandparent->num_of_children--;
    return 0;
}

static int merge(bpt_t *t, bpt_node_t *parent, char *key, char *split_key) {
    // remove the split key
    unsigned long long i = 0;
    for (i = 0; i < parent->num_of_keys; i++) {
        if (strcmp(parent->keys[i], split_key) == 0) {
            unsigned long long j = 0;
            for (j = i; j <= parent->num_of_keys - 1; j++) {
                parent->keys[j] = parent->keys[j+1];
            }
            break;
        }
    }
    // parent has enough keys
    parent->num_of_keys--;
    unsigned long long num_of_keys = parent->num_of_keys;

    if (num_of_keys >= DEGREE / 2 || (bpt_is_root(parent) && num_of_keys >= 1))
        return 1;

    if (bpt_is_root(parent) && num_of_keys == 0) {
        t->root = parent->children[0];
        parent->children[0]->parent = NULL;
        bpt_free_non_leaf(parent);
        return 1;
    }
    // now we have to find a split key for parent
    bpt_node_t *grandparent = parent->parent;
    for (i = 0; i < grandparent->num_of_children; i++) {
        if (grandparent->children[i] == parent)
            break;
    }
    unsigned long long split = (i == 0) ? 0 : i - 1;
    split_key = grandparent->keys[split];

    // 1 means redistribution is done, no more recursion
    if (merge_internal(t, parent, split_key) == 1)
        return 1;
    merge(t, parent->parent, key, split_key);
    return 1;
}

// this functin first finish leaf merging
// then call merge to see if it is necessary to merge parent
/*
  procedure:
      1. find leaf and internal node which contains the key to be deleted
      2. find split key
      3. replace the key in the internal node with the split key
      4. merge leaf and its chosen sibling, 
      5. remove the split key in parent
      6. check if we need to merge parent recursively, if so , we have to 
         incorporate the split key which split parent and parent's sibling
      7. if parent is root and we have to remove the last key during merging,
         just remove the key and delete this root. Delegate root to the newly
         merged node
 */
static int bpt_complex_delete(bpt_t *t, bpt_node_t *leaf, char *key) {
    // find internal node which contains the keys to be deleted
    bpt_node_t *non_leaf = find_non_leaf(t, key);
    
    // find split key
    bpt_node_t *parent = leaf->parent; // impossible to be NULL
    unsigned long long i = 0;
    for (i = 0; i < parent->num_of_children; i++) {
        if (parent->children[i] == leaf)
            break;
    }
    unsigned long long split = (i == 0) ? 0 : i - 1;
    char *split_key = parent->keys[split];
    // replace the key
    if (non_leaf) {
        for (unsigned long long j = 0; j < non_leaf->num_of_keys; j++)
            if (strcmp(non_leaf->keys[j], key) == 0) {
                non_leaf->keys[j] = split_key;
                break;
            }
    }
    // merge
    merge_leaves(t, leaf, key);
    merge(t, parent, key, split_key);
    free(t->free_key);
    t->free_key = NULL;
    return 1;
}

int bpt_delete(bpt_t *t, char *key) {
    unsigned long long  i = 0; 
    bpt_node_t *leaf = find_leaf(t, key);
    for (i = 0; i < leaf->num_of_keys; i++) {
        if (strcmp(leaf->keys[i], key) == 0)
            break;
    }

    if (i == leaf->num_of_keys)
        return 1;
    
    bpt_node_t *parent = leaf->parent;
    if (leaf->num_of_keys > DEGREE / 2 || bpt_is_root(leaf)) {
        bpt_simple_delete(t, leaf, key);

        // tree is destroyed
        if (leaf->num_of_keys == 0) {
            bpt_free_leaf(leaf);
            t->root = NULL;
        }

        // index key should be replaced it is resides in parent's key list
        if (parent) {
            for (i = 0; i < parent->num_of_keys; i++) {
                if (strcmp(parent->keys[i], key) == 0) {
                    parent->keys[i] = leaf->keys[0];
                    return 1;
                }
            }
        }
    } else {
        // if one of leaf's nearest siblings has enough keys to share
        // borrow a key from this sibling
        if (bpt_check_redistribute(leaf))
            return redistribute_leaf(t, leaf, key);
        else
            // no sibling can offer us a key
            // merge is required
            return bpt_complex_delete(t, leaf, key);
    }
    return 1;
}
