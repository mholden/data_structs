#include "include/linked_list.h"
#include <stdlib.h>
#include <stdio.h>

/* Insert key-value pair into list */
static int ll_insert(struct ll_node **root, int key, int value){
	struct ll_node *node;

	node = *root;
	if(node == NULL){ /* Inserting at root */
		*root = malloc(sizeof(struct ll_node));
        	if(*root == NULL){
                	printf("ll_insert(): Out of memory?\n");
                	return -1;
        	}

		node = *root;

        	goto init_node;
	}
	
	while(node->next != NULL){
		if(node->key == key){ /* Key already exists */
			printf("ll_insert(): key already exists.\n");
			return -1;
		}
		else node = node->next;
	}

	node->next = malloc(sizeof(struct ll_node));
	if(node->next == NULL){
		printf("ll_insert(): Out of memory?\n");
		return -1;
	}

	node = node->next;

init_node:
	node->key = key;
	node->value = value;
	node->next = NULL;

	return 0;
}

/* Given key, find value in list */
static int ll_find(struct ll_node *root, int key, int *value){
	struct ll_node *node;
	int val;

	node = root;
	while(node != NULL){
		if(node->key == key){
			*value = node->value;
			return 0;
		}
		else node = node->next;
	}

	//printf("ll_find(): Key doesn't exist.\n");
	return -1;
}

/* Remove key-value pair from list */
static int ll_remove(struct ll_node *root, int key){
	/* IMPLEMENT */
	return 0;
}

static struct ll_ops ll_standard_ops = {
	ll_insert,
	ll_find,
	ll_remove
};

struct linked_list *ll_create(){
	struct linked_list *new_ll;
	
	new_ll = malloc(sizeof(struct linked_list));
	if(new_ll == NULL){
		printf("ll_create(): Out of memory?\n");
		return NULL;
	}

	new_ll->root = NULL;
	new_ll->ops = &ll_standard_ops;

	return new_ll;
}

void ll_destroy(struct linked_list *ll){
	struct ll_node *node, *prev;
	
	node = ll->root;
	prev = NULL;
	while(node != NULL){
		prev = node;	
		node = node->next;
		free(prev);
	}

	free(ll);
	return;
}
