#include "heap.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

heap_t *heap_create(heap_ops_t *hops) {
    heap_t *new_heap = NULL;
    
    new_heap = malloc(sizeof(heap_t));
    if (!new_heap) {
        printf("heap_create: couldn't malloc new_heap\n");
        goto error_out;
    }
    
    new_heap->h_ops = hops;
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
        h->h_ops->heap_destroy_element(h->h_heap[i]);
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
    
    if (h->h_ops->heap_compare(parent, child) < 0) { // swap and continue
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

static void heap_downheap(heap_t *h, int ind) {
    int cind1, cind2, scind = -1;
    void *parent, *child1 = NULL, *child2 = NULL, *swap_child;
    
    if (h->h_nelements == 0) return;
    
    assert((ind >= 0) && (ind < h->h_nelements));
    
    cind1 = (ind * 2) + 1;
    cind2 = cind1 + 1;
    
    parent = h->h_heap[ind];
    if (cind1 < h->h_nelements)
        child1 = h->h_heap[cind1];
    if (cind2 < h->h_nelements)
        child2 = h->h_heap[cind2];
    
    if (child1 == NULL && child2 == NULL)
        return;
    
    if (child1 == NULL)
        scind = cind2;
    else if (child2 == NULL)
        scind = cind1;
    
    if (scind < 0) { // neither child1 nor child2 are NULL
        if (h->h_ops->heap_compare(child1, child2) > 0)
            scind = cind1;
        else
            scind = cind2;
    }
    
    swap_child = h->h_heap[scind];
    
    if (h->h_ops->heap_compare(swap_child, parent) > 0) { // swap and continue
        h->h_heap[ind] = swap_child;
        h->h_heap[scind] = parent;
        heap_downheap(h, scind);
    }
    
    return;
}

void *heap_top(heap_t *h) {
    return h->h_heap[0];
}

void *heap_pop(heap_t *h) {
    void *top = heap_top(h);
    
    if (h->h_nelements == 0)
        goto out;
    
    h->h_heap[0] = h->h_heap[h->h_nelements-1];
    h->h_heap[h->h_nelements-1] = NULL;
    h->h_nelements--;
    heap_downheap(h, 0);
    
out:
    return top;
}

bool heap_empty(heap_t *h) {
    return (h->h_nelements == 0);
}

void heap_dump(heap_t *h) {
    printf("** heap @ %p **\n", h);
    printf("h_size: %d\n", h->h_size);
    printf("h_nelements: %d\n", h->h_nelements);
    printf("h_heap:\n");
    for (int i = 0; i < h->h_nelements; i++) {
        printf("[%d]: %p ", i, h->h_heap[i]);
        h->h_ops->heap_dump_element(h->h_heap[i]);
        //printf("\n");
    }
}
