#ifndef _HEAP_H_
#define _HEAP_H_

#include <stddef.h>

#define HEAP_START_SIZE 64

typedef struct heap_ops {
    int (*heap_compare)(void *element1, void *element2);
    int (*heap_destroy_element)(void *element);
    void (*heap_dump_element)(void *element, int ind);
} heap_ops_t;

typedef struct heap {
    size_t h_size;
    heap_ops_t *h_ops;
    void **h_heap;
    size_t h_nelements;
} heap_t;

heap_t *heap_create(heap_ops_t *hops);
void heap_destroy(heap_t *h);

int heap_insert(heap_t *h, void *element);
void *heap_top(heap_t *h);
void *heap_pop(heap_t *h);

void heap_dump(heap_t *h);

#endif /* _HEAP_H_ */
