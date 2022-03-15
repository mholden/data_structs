#ifndef _HEAP_H_
#define _HEAP_H_

#include <stddef.h>

#define HEAP_START_SIZE 64

typedef int (*heap_compare_fn_t)(void *element1, void *element2);
typedef int (*heap_destroy_element_fn_t)(void *element);
typedef void (*heap_dump_element_fn_t)(void *element, int ind);

typedef struct heap {
    size_t h_size;
    heap_compare_fn_t h_compare_fn;
    heap_destroy_element_fn_t h_destroy_element_fn;
    heap_dump_element_fn_t heap_dump_element_fn;
    void **h_heap;
    size_t h_nelements;
} heap_t;

heap_t *heap_create(heap_compare_fn_t hcf, heap_destroy_element_fn_t hdef, heap_dump_element_fn_t hduef);
void heap_destroy(heap_t *h);

int heap_insert(heap_t *h, void *element);
void *heap_top(heap_t *h);
int heap_remove(heap_t *h, void *element);

void heap_dump(heap_t *h);

#endif /* _HEAP_H_ */
