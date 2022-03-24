#ifndef _QUEUE_H_
#define _QUEUE_H_

typedef struct queue {
	int head_ind;
	int tail_ind;
	int curr_size;
	int max_size;
	void **data;
} queue_t;

struct queue *q_create(int size);
int q_empty(struct queue *q);
int q_add(struct queue *q, void *new_entry);
int q_get(struct queue *q, void **entry);
void q_destroy(struct queue *q);

void q_dump(queue_t *q, void (*callback)(void *data));

#endif /* _QUEUE_H_ */
