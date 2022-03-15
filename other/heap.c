#include "include/heap.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

heap_t *heap_create(heap_compare_fn_t hcf, heap_destroy_element_fn_t hdef, heap_dump_element_fn_t hduef) {
    heap_t *new_heap = NULL;
    
    new_heap = malloc(sizeof(heap_t));
    if (!new_heap) {
        printf("heap_create: couldn't malloc new_heap\n");
        goto error_out;
    }
    
    new_heap->h_compare_fn = hcf;
    new_heap->h_destroy_element_fn = hdef;
    new_heap->heap_dump_element_fn = hduef;
    new_heap->h_size = HEAP_START_SIZE;
    new_heap->h_heap = malloc(HEAP_START_SIZE * sizeof(void *));
    if (!new_heap->h_heap) {
        printf("heap_create: couldn't malloc h_heap\n");
        goto error_out;
    }
    memset(new_heap->h_heap, 0, new_heap->h_size * sizeof(void *));
    new_heap->h_nelements = 0;
    
out:
    return new_heap;
    
error_out:
    if (new_heap)
        free(new_heap);
    
    return NULL;
}

void heap_destroy(heap_t *h) {
    for (int i = 0; i < h->h_nelements; i++)
        h->h_destroy_element_fn(h->h_heap[i]);
    free(h->h_heap);
    free(h);
}

static void heap_upheap(heap_t *h, size_t ind) {
    int pind;
    void *child, *parent;
    
    if (ind == 0) return;
    
    pind = (ind - 1) / 2;
    child = h->h_heap[ind];
    parent = h->h_heap[pind];
    
    if (h->h_compare_fn(parent, child) < 0) { // swap and continue
        h->h_heap[ind] = parent;
        h->h_heap[pind] = child;
        heap_upheap(h, pind);
    }
    
    return;
}

int heap_insert(heap_t *h, void *element) {
    int err = 0;
    
    if (h->h_nelements == h->h_size) { // heap is full, make it larger
        void **p = realloc(h->h_heap, (h->h_size + HEAP_START_SIZE) * sizeof(void *));
        if (!p) {
            printf("heap_insert: realloc failed\n");
            goto error_out;
        }
        h->h_heap = p;
        h->h_size += HEAP_START_SIZE;
    }
    
    h->h_heap[h->h_nelements] = element;
    heap_upheap(h, h->h_nelements);
    h->h_nelements++;
    
    return 0;
    
error_out:
    return -1;
}

// TODO: implement
void *heap_top(heap_t *h) {
    
}

// TODO: implement
int heap_remove(heap_t *h, void *element) {
    return 0;
}

void heap_dump(heap_t *h) {
    printf("** heap @ %p **\n", h);
    printf("h_size: %d\n", h->h_size);
    printf("h_nelements: %d\n", h->h_nelements);
    printf("h_heap:\n");
    for (int i = 0; i < h->h_nelements; i++) {
        h->heap_dump_element_fn(h->h_heap[i], i);
    }
}
