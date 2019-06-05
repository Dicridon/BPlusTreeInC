#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include "bplustree.h"

// #define POOL_SIZE (((long)2 * 1024 * 1024 * 1024))
#define K ((long long)1024)
#define M (K * K)
#define G (K * K * K)
#define POOL_SIZE (3 * G)
// #define DEBUG
#define PERF

#define DN ((long)26)
#define PRE ((long)2000000)
#define K_LEN ((long)20)
#define V_LEN ((long)128)

char dict[DN] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
                 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
                 'y', 'z',
};

void query(bpt_t *t, char **cases, long size) {
    clock_t start, end;
    printf("querying 50\%% keys...\n");
    srand(size / 233 * 45 > 4);
    long s  = rand() % size;
    start = clock();
    for (long i = 0; i < size / 2; i++) {
        bpt_get(t, cases[(s++) % size], NULL);
    }
    end = clock();
    printf("time consumed in querying %lf, %lf ops per second\n",
           (double)(end - start) / CLOCKS_PER_SEC,
           (size/ 2) / (((double)(end - start)) / CLOCKS_PER_SEC));

    printf("querying 90\%% keys...\n");
    srand(size / 233 * 45 > 4);
    s  = rand() % size;
    start = clock();
    for (long i = 0; i < size * 9 / 10; i++) {
        bpt_get(t, cases[(s++) % size], NULL);
    }
    end = clock();
    printf("time consumed in querying %lf, %lf ops per second\n",
           (double)(end - start) / CLOCKS_PER_SEC,
           (size / 10 * 9) / (((double)(end - start)) / CLOCKS_PER_SEC));

    printf("querying all keys...\n");
    srand(size / 233 * 45 > 4);
    start = clock();
    for (long i = 0; i < size; i++) {
        bpt_get(t, cases[i], NULL);
    }
    end = clock();
    printf("time consumed in querying %lf, %lf ops per second\n",
           (double)(end - start) / CLOCKS_PER_SEC,
           (size) / (((double)(end - start)) / CLOCKS_PER_SEC));
}

void scan(bpt_t *t, const char *key, long size) {
    int scale[] = {5, 10, 25, 50, 75, 100};
    clock_t start, end;
    printf("scanning 1\%% keys starting with %s\n", key);
    
    start = clock();
    bpt_range_test(t, key, size / 100);
    end = clock();
    printf("time consumed in scanning: %lf, %lf ops per second\n",
           (double)(end - start) / CLOCKS_PER_SEC,
           (size / 100) / (((double)(end - start)) / CLOCKS_PER_SEC));

    for (int i = 0; i < 6; i++) {
        printf("scanning %d\%% keys(%ld) starting with %s\n",
               scale[i], size / 100 * scale[i], key);
        start = clock();
        bpt_range_test(t, key, size / 100 * scale[i]);
        end = clock();
        printf("time consumed in scanning: %lf, %lf ops per second\n",
               (double)(end - start) / CLOCKS_PER_SEC,
               (size / 100 * scale[i]) / (((double)(end-start)) / CLOCKS_PER_SEC));
    }
}

char *generate_key(long seed) {
    srand(seed);
    long length = K_LEN;

    char * str = malloc(length+1);
    memset(str, 0, length+1);
    for (int i = 0; i < length; i++) {
        str[i] = dict[rand() % DN];
    }
    return str;
}

char *generate_value(long seed) {
    srand(seed);
    long length = V_LEN;

    char * str = malloc(length+1);
    memset(str, 0, length+1);
    for (int i = 0; i < length; i++) {
        str[i] = dict[rand() % DN];
    }
    return str;
}

void print_keys(struct bpt_node *node) {
    for (unsigned long long i = 0; i < node->num_of_keys; i++) {
        printf("%s, ", node->keys[i]);
    }
}

void print_children(struct bpt_node *parent) {
    for (unsigned long long i = 0; i < parent->num_of_children; i++) {
        printf("%p, ", parent->children[i]);
    }
}
bool validate(bpt_t *t, struct bpt_node *node) {
    if (node == NULL) {
        if (!t->root) {
            printf("null inside B+ tree\n"); 
            return false;            
        } else {
            return true;
        }
    }

    if (node->type == NON_LEAF) {
        struct bpt_node* child = NULL;
        // if (t->root != node && node->num_of_keys < DEGREE / 2) {
        //     printf("%p has less than %d keys\n", node, DEGREE / 2);
        //     print_keys(node);
        //     print_children(node);
        //     puts("");
        //     return false;
        // }

        if (node->num_of_children != node->num_of_keys + 1) {
            printf("this node %p is illegal: ", node);
            printf("num_of_children = %llu, num_of_keys = %llu\n",
                   node->num_of_children, node->num_of_keys);
            printf("keys are: ");
            print_keys(node);
            puts("");
            return false;
        }

        for (unsigned long long i = 0; i < node->num_of_keys; i++) {
            child = node->children[i];
            if (strcmp(node->keys[i],
                       child->keys[child->num_of_keys-1]) <= 0) {
                printf("illegal child %p\n", child);
                printf("parent keys: ");
                print_keys(node);
                puts("");
                printf("children of parent\n");
                print_children(node);
                puts("");
                printf("child keys: ");
                print_keys(child);
                puts("");
                return false;
            }
        }
        child = node->children[node->num_of_children - 1];
        if (strcmp(node->keys[node->num_of_keys-1], child->keys[0]) > 0) {
            printf("illegal child %p\n", child);
            printf("parent keys: ");
            print_keys(node);
            puts("");
            printf("children of parent\n");
            print_children(node);
            puts("");
            printf("child keys: ");
            print_keys(child);
            puts("");
            return false;
        }
        
        for (unsigned long long i = 0; i < node->num_of_children; i++)
            if (!validate(t, node->children[i]))
                return false;
    }
    return true;
}



int run_test(bpt_t *t, long size) {
    char **keys;
    char **values;
    clock_t start, end;
    keys = malloc(size * sizeof(char *));
    values = malloc(size * sizeof(char *));
    printf("\n****** Preparing %ld data, please wait... ******\n", size);
    for (long i = 0; i < size; i++) {
        // temp = generate_key(i + 1323);
        // temp = generate_key(size + i * 33 / 45 << 3 - 147);
        keys[i] = generate_key(i + 1323);
        values[i] = generate_value(i + 1323);
    }


    // Insertion test
    printf("Insertion started, please wait...\n");
    // warm up
    for (long i = 0; i < size / 2; i++) {
#ifdef DEBUG
        printf("inserting %s, %ld/%ld\n", keys[i], i + 1, size);
#endif
//         if (i % 10000)
//             printf("%ld\%%/%ld finished\n", i / 10000, size / 10000);
        bpt_insert(t, keys[i], values[i]);
    }

    puts("Warm up finished");
    // bpt_print(t);
    // test starts here
    start = clock();
    for (long i = size / 2; i < size; i++) {
#ifdef DEBUG
        printf("inserting %s, %ld/%ld\n", keys[i], i + 1, size);
#endif
        bpt_insert(t, keys[i], values[i]);
    }
    end = clock();
#ifndef PERF
    if (!validate(t, &D_RW(t->t)->root)) {
        bpt_print(t);
            
        printf("!!!! VALIDATION FAILED DURING INSERTION\n");
        bpt_print(t); 
        return -1;
    }
#endif
    printf("time consumed in insertion %lf, %lf ops per second\n",
           (double)(end - start) / CLOCKS_PER_SEC,
           (size / 2) / (((double)(end - start)) / CLOCKS_PER_SEC));

    // Query test
    query(t, keys, size);

    // scanning
    scan(t, "aaaaaaaaaa", size);
    
    // Deletion test
    printf("Deletion started, please wait...\n");
    start = clock();
    for (long i = 0; i < size; i++) {
#ifdef DEBUG
        printf("deleting %s, %ld/%ld\n", keys[i], i + 1, size);
#endif
        bpt_delete(t, keys[i]);
#ifdef DEBUG
        if (bpt_get(t, keys[i], NULL) != -1) {
            printf("deleting %s failed\n", keys[i]);
            assert(0);
        }
#endif
            
#ifndef PERF
        if (i % 1000 == 0) {
            if (!validate(t, &D_RW(t->t)->root)) {  
                bpt_print(t);
                printf("!!!! VALIDATION FAILED WHEN DELETING %s, %ld/%ld\n",
                       keys[i], i+1, size);
                assert(0);
            }
        }
#endif
    }
    end = clock();
    printf("time consumed in deletion %lf, %lf ops per second\n",
           (double)(end - start) / CLOCKS_PER_SEC,
           (size) / (((double)(end - start)) / CLOCKS_PER_SEC));


    if (t->root == NULL) {
        puts("PASS!!!");
        printf("You successfully covered %ld keys\n\n", size);
    } else {
	printf("tree is not empty after deletion\n\n");
        bpt_print(t);
    }

    for (long i = 0; i < size; i++) {
        free(keys[i]);
        free(values[i]);
    }
    free(keys);
    free(values);
    bpt_destroy(t);
    return 1;
}

int main() {
    bpt_t *t;

    for (int i = 1; i <= 10; i++) {
        t = bpt_new();
        run_test(t, PRE / 10 * i);
    }
}
