#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include "heap.h"

typedef struct heap_element {
    int he_key;
} heap_element_t;

static int test_heap_compare(void *element1, void *element2) {
    heap_element_t *he1, *he2;
    
    he1 = (heap_element_t *)element1;
    he2 = (heap_element_t *)element2;
    
    if (he1->he_key < he2->he_key)
        return -1;
    else if (he1->he_key > he2->he_key)
        return 1;
    else // ==
        return 0;
}

static int test_heap_destroy_element(void *element) {
    heap_element_t *he = (heap_element_t *)element;
    assert(he);
    free(he);
}

static void test_heap_dump_element(void *element, int ind) {
    heap_element_t *he = (heap_element_t *)element;
    printf(" he_key[%d]: %d\n", ind, he->he_key);
}

static heap_ops_t test_heap_ops = {
    .heap_compare = test_heap_compare,
    .heap_destroy_element = test_heap_destroy_element,
    .heap_dump_element = test_heap_dump_element
};

int main(int argc, char **argv) {
    heap_t *h;
    heap_element_t *he, *he_next;
    int err = -1, num_elements;
    
    if (argc != 2) {
        printf("usage: %s <num-elements>\n", argv[0]);
        return -1;
    }
    
    num_elements = (int)strtol(argv[1], NULL, 10);
    
    h = heap_create(&test_heap_ops);
    if (!h) {
        printf("heap_create failed\n");
        goto error_out;
    }
    
    srand(time(NULL));
    
    // insert some elements
    for (int i = 0; i < num_elements; i++) {
        he = malloc(sizeof(heap_element_t));
        if (!he) {
            printf("malloc failed to create heap element\n");
            goto error_out;
        }
        he->he_key = rand();
        err = heap_insert(h, he);
        if (err) {
            printf("heap_add failed\n");
            goto error_out;
        }
    }
    
    //heap_dump(h);
    
    // pop elements
    for (int i = 0; i < num_elements; i++) {
        he = heap_pop(h);
        he_next = heap_top(h);
        if (he_next)
            assert(h->h_ops->heap_compare(he, he_next) >= 0);
        //test_heap_dump_element(he, 0);
        test_heap_destroy_element(he);
    }
    
    assert(h->h_nelements == 0);
    
    heap_destroy(h);
    
    return 0;
    
error_out:
    if (h)
        heap_destroy(h);
    
    return err;
}
