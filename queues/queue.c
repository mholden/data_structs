#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

struct queue *q_create(int size){
	struct queue *q;
	int i;

	q = malloc(sizeof(struct queue));
	if(q == NULL){
		printf("q_create(): malloc() error.\n");
		return NULL;
	}

	q->head_ind = 0;
	q->tail_ind = 0;
	q->curr_size = 0;
	q->max_size = size;

	q->data = malloc(size * sizeof(void *));
	if(q->data == NULL){
                printf("q_create(): malloc() error.\n");
                return NULL;
        }

	for(i=0; i<size; i++) q->data[i] = NULL;

	return q;
}

int q_empty(struct queue *q){
	return !q->curr_size;
}

int q_add(struct queue *q, void *new_entry){
	if(q->curr_size == q->max_size){
		printf("q_add(): queue is already full.\n");
		return -1;
	}

	if(!q_empty(q)) q->tail_ind = (q->tail_ind + 1) % q->max_size;
	q->data[q->tail_ind] = new_entry;
	q->curr_size++;

	return 0;
}

int q_get(struct queue *q, void **entry){
	if(q_empty(q)){
		printf("q_get(): queue is empty.\n");
		return -1;
	}

	*entry = q->data[q->head_ind];
        q->data[q->head_ind] = NULL;
	if(q->curr_size != 1) q->head_ind = (q->head_ind + 1) % q->max_size;
	q->curr_size--;

	return 0;
}

void q_destroy(struct queue *q){
	free(q->data);
	free(q);

	return;
}

void q_dump(queue_t *q, void (*callback)(void *data)) {
    int i;
    printf("head_ind: %d\n", q->head_ind);
    printf("tail_ind: %d\n", q->tail_ind);
    printf("curr_size: %d\n", q->curr_size);
    printf("max_size: %d\n", q->max_size);
    printf("data:\n");
    i = q->head_ind;
    for (int j = 0; j < q->curr_size; j++) {
        printf("[%d]:", i);
        callback(q->data[i]);
        printf("\n");
        i = (i + 1) % q->max_size;
    }
}
