#ifndef _FIFO_H_
#define _FIFO_H_

#include <stddef.h>
#include <stdbool.h>

typedef struct fifo_ops {
    void (*fo_destroy_data_fn)(void *data);
    void (*fo_dump_data_fn)(void *data);
} fifo_ops_t;

typedef struct fifo {
	int fi_head;
	int fi_tail;
	size_t fi_size;
	size_t fi_nelements;
	void **fi_queue;
    fifo_ops_t *fi_ops;
} fifo_t;

#define FIFO_START_SIZE 1024

fifo_t *fi_create(fifo_ops_t *fops);
void fi_destroy(fifo_t *fi);

int fi_enq(fifo_t *fi, void *data);
int fi_deq(fifo_t *fi, void **data);

bool fi_full(fifo_t *fi);
bool fi_empty(fifo_t *fi);

void fi_dump(fifo_t *fi);
void fi_check(fifo_t *fi);

#endif /* _FIFO_H_ */
