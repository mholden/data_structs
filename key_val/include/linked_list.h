#ifndef _LINKED_LIST_H_
#define _LINKED_LIST_H_

/*
 * Linked list definitions.
 */
#include <stdint.h>

struct ll_node{
	uint64_t key;
	uint64_t value;
	struct ll_node *next;
};

struct ll_ops{
	int (*insert)(struct ll_node **root, uint64_t key, uint64_t value);
	int (*find)(struct ll_node *root, uint64_t key, uint64_t *value);
	int (*remove)(struct ll_node *root, uint64_t key);
};

struct linked_list{
	struct ll_node *root;
	struct ll_ops *ops;
};

/* Create a linked list with standard ops */
struct linked_list *ll_create(void);

/* Destroy a linked list */
void ll_destroy(struct linked_list *ll);

#endif /* _LINKED_LIST_H_ */
