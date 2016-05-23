#include "include/binary_tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int _bt_insert(struct bt_node **node, int (*key_compare)(void *key1, void *key2), void *key, size_t keysz, void *value, size_t valsz){
    struct bt_node *_node;
    int ret = 0;
    
    _node = *node;
    if(_node == NULL){ /* Do the insert */
        _node = malloc(sizeof(struct bt_node));
        if(_node == NULL){
            printf("bt_insert(): Out of memory?\n");
            return -1;
        }
        _node->key = malloc(keysz);
        _node->value = malloc(valsz);
        if (_node->key == NULL || _node->value == NULL){
            printf("_bt_insert(): Out of memory?\n");
            return -1;
        }
        memcpy(_node->key, key, keysz);
        memcpy(_node->value, value, valsz);
        _node->lchild = NULL;
        _node->rchild = NULL;
        
        *node = _node;
        
        return 0;
    }
    
    int keycmp = key_compare(_node->key, key);
    if(keycmp > 0) ret = _bt_insert(&_node->rchild, key_compare, key, keysz, value, valsz);
    else if(keycmp < 0) ret = _bt_insert(&_node->lchild, key_compare, key, keysz, value, valsz);
    else{ /* Key already exists */
        printf("bt_insert(): key already exists.\n");
        ret = -1;
    }
    
    return ret;
}

int bt_insert(struct binary_tree *bt, void *key, void *value){
	int ret;

    ret = _bt_insert(&bt->root, bt->key_compare, key, bt->keysz, value, bt->valsz);

	return ret;
}

static int _bt_find(struct bt_node *node, int (*key_compare)(void *key1, void *key2), void *key, size_t keysz, void *value, size_t valsz){
    int ret;
    
    if (node == NULL) {
        printf("bt_find(): key doesn't exist.\n");
        return -1;
    }
    
    int keycmp = key_compare(node->key, key);
    if (keycmp == 0) {
        memcpy(value, node->value, valsz);
        return 0;
    }
    else if (keycmp > 0)
        ret = _bt_find(node->rchild, key_compare, key, keysz, value, valsz);
    else
        ret = _bt_find(node->lchild, key_compare, key, keysz, value, valsz);
    
    return ret;
}

int bt_find(struct binary_tree *bt, void *key, void *value){
	int ret;

    ret = _bt_find(bt->root, bt->key_compare, key, bt->keysz, value, bt->valsz);
	
	return ret;
}

int bt_remove(struct bt_node *root, void *key){
	/* IMPLEMENT */
	return 0;
}

struct binary_tree *bt_create(size_t keysz, size_t valsz, int (*key_compare)(void *key1, void *key2)){
	struct binary_tree *bt;

	bt = malloc(sizeof(struct binary_tree));
	if(bt == NULL){
		printf("bt_create(): Out of memory?\n");
		return NULL;
	}

	bt->root = NULL;
    bt->key_compare = key_compare;
    bt->keysz = keysz;
    bt->valsz = valsz;

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
