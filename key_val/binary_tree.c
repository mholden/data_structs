#include "include/binary_tree.h"
#include <stdio.h>
#include <stdlib.h>

static int bt_insert(struct bt_node **root, int key, int value){
	struct bt_node *node;
	int ret;

	node = *root;
	if(node == NULL){ /* Do the insert */
		node = malloc(sizeof(struct bt_node));
		if(node == NULL){
			printf("bt_insert(): Out of memory?\n");
			return -1;
		}
		node->key = key;
		node->value = value;
		node->lchild = NULL;
		node->rchild = NULL;

		*root = node;
		
		return 0;
	}

	if(key > node->key) ret = bt_insert(&node->rchild, key, value);
	else if(key < node->key) ret = bt_insert(&node->lchild, key, value);
	else{ /* Key already exists */
		printf("bt_insert(): key already exists.\n");
		ret = -1;
	}

	return ret;
}

static int bt_find(struct bt_node *root, int key, int *value){
	struct bt_node *node;
	int ret;

	node = root;
	if(node == NULL){
		printf("bt_find(): key doesn't exist.\n");
		return -1;
	}

	if(node->key == key){
		*value = node->value;
		return 0;
	}
	else if(key > node->key) ret = bt_find(node->rchild, key, value);
	else ret = bt_find(node->lchild, key, value);
	
	return ret;
}

static int bt_remove(struct bt_node *root, int key){
	/* IMPLEMENT */
	return 0;
}

struct bt_ops bt_standard_ops = {
	bt_insert,
	bt_find,
	bt_remove
};

struct binary_tree *bt_create(void){
	struct binary_tree *bt;

	bt = malloc(sizeof(struct binary_tree));
	if(bt == NULL){
		printf("bt_create(): Out of memory?\n");
		return NULL;
	}

	bt->root = NULL;
	bt->ops = &bt_standard_ops;

	return bt;
}

static void destroy_tree(struct bt_node *tree_root){
	if(tree_root == NULL) return;
	
	destroy_tree(tree_root->lchild);
	destroy_tree(tree_root->rchild);

	free(tree_root);

	return;
}

void bt_destroy(struct binary_tree *bt){
	struct bt_node *tree_root;

	tree_root = bt->root;
	destroy_tree(tree_root);

	free(bt);

	return;
}
