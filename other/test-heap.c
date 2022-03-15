#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "include/heap.h"

typedef struct heap_element {
    int he_key;
} heap_element_t;

int heap_compare(void *element1, void *element2) {
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

int heap_destroy_element(void *element) {
    heap_element_t *he = (heap_element_t *)element;
    free(he);
}

void heap_dump_element(void *element, int ind) {
    heap_element_t *he = (heap_element_t *)element;
    printf(" he_key[%d]: %d\n", ind, he->he_key);
}

int main(int argc, char **argv) {
    heap_t *h;
    heap_element_t *he;
    int err = -1;
    
    h = heap_create(heap_compare, heap_destroy_element, heap_dump_element);
    if (!h) {
        printf("heap_create failed\n");
        goto error_out;
    }
    
    srand(time(NULL));
    
    // insert some elements
    for (int i = 0; i < 128; i++) {
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
    
#if 0
    heap_dump(h);
#endif
    
    heap_destroy(h);
    
    return 0;
    
error_out:
    if (h)
        heap_destroy(h);
    
    return err;
}
