#ifndef _BINARY_TREE_H
#define _BINARY_TREE_H

#include <stdlib.h>

/*
 * Binary Tree.
 */

struct bt_node{
	void *key;
	void *value;
	struct bt_node *lchild;
	struct bt_node *rchild;
};

struct binary_tree{
	struct bt_node *root;
    int (*key_compare)(void *key1, void *key2);
    size_t keysz;
    size_t valsz;
};

struct binary_tree *bt_create(size_t keysz, size_t valsz, int (*key_compare)(void *key1, void *key2));
int bt_insert(struct binary_tree *bt, void *key, void *value);
int bt_find(struct binary_tree *bt, void *key, void *value);
int bt_remove(struct bt_node *root, void *key);
void bt_destroy(struct binary_tree *bt);

#endif /* _BINARY_TREE_H_ */
