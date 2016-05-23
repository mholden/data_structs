#include "include/linked_list.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Insert (/update) key-value pair into list */
int ll_insert(struct linked_list *ll, void *key, void *value, int flags){
	struct ll_node *node, *prev;

	node = ll->root;
	if (node == NULL) { /* Inserting at root */
		ll->root = malloc(sizeof(struct ll_node));
        if(ll->root == NULL){
            printf("ll_insert(): Out of memory?\n");
            return -1;
        }
        
        node = ll->root;
        
        goto init_node;
	}
	
	do {
		if(!ll->key_compare(node->key, key)) /* Key already exists - update it */
			goto update_node;
		
		prev = node;
		node = node->next;
	} while (node);

	node = prev;
	node->next = malloc(sizeof(struct ll_node));
	if (node->next == NULL) {
		printf("ll_insert(): Out of memory?\n");
		return -1;
	}

	node = node->next;

init_node:
	node->key = malloc(ll->keysz);
    if (flags & LL_NO_ALLOC)
        node->value = value;
    else
        node->value = malloc(ll->valsz);
    if (node->key == NULL || node->value == NULL) {
        printf("ll_insert(): Out of memory?\n");
        return -1;
    }
	node->next = NULL;
    
    memcpy(node->key, key, ll->keysz);
	
update_node:
	memcpy(node->value, value, ll->valsz);

	return 0;
}

/* Given key, find value in list */
int ll_find(struct linked_list *ll, void *key, void *value){
	struct ll_node *node;

	node = ll->root;
	while (node != NULL) {
		if (!ll->key_compare(node->key, key)) {
			memcpy(value, node->value, ll->valsz);
			return 0;
		}
		else node = node->next;
	}

	return -1;
}

/* Remove key-value pair from list */
int ll_remove(struct linked_list *ll, void *key, int flags){
    struct ll_node *node, *prev;
    
    prev = NULL;
    node = ll->root;
    while (node != NULL) {
        if (!ll->key_compare(node->key, key))
            break;
        prev = node;
        node = node->next;
    }
    
    if (node == NULL) // key doesn't exist in list
        return -1;
    
    prev->next = node->next;
    free(node->key);
    if (!(flags & LL_NO_FREE))
        free(node->value);
    
	return 0;
}

struct linked_list *ll_create(size_t keysz, size_t valsz, int (*key_compare)(void *key1, void *key2)) {
	struct linked_list *new_ll;
	
	new_ll = malloc(sizeof(struct linked_list));
	if(new_ll == NULL){
		printf("ll_create(): Out of memory?\n");
		return NULL;
	}

	new_ll->root = NULL;
    new_ll->keysz = keysz;
    new_ll->valsz = valsz;
	new_ll->key_compare = key_compare;

	return new_ll;
}

void ll_destroy(struct linked_list *ll){
	struct ll_node *node, *prev;
	
	node = ll->root;
	prev = NULL;
	while(node != NULL){
		prev = node;	
		node = node->next;
        free(prev->key);
        free(prev->value);
		free(prev);
	}

	free(ll);
	return;
}
