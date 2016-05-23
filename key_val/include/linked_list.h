#ifndef _LINKED_LIST_H_
#define _LINKED_LIST_H_

/*
 * Linked list.
 */
#include <stdint.h>

struct ll_node{
	void *key;
	void *value;
	struct ll_node *next;
};

struct linked_list{
	struct ll_node *root;
    size_t keysz;
    size_t valsz;
    int (*key_compare)(void *key1, void *key2);
};

struct linked_list *ll_create(size_t keysz, size_t valsz, int (*key_compare)(void *key1, void *key2));
int ll_insert(struct linked_list *ll, void *key, void *value);
int ll_find(struct linked_list *ll, void *key, void *value);
void ll_destroy(struct linked_list *ll);

#endif /* _LINKED_LIST_H_ */
