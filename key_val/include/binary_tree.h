#ifndef _BINARY_TREE_H
#define _BINARY_TREE_H

/*
 * Binary Tree definitions.
 */

struct bt_node{
	int key;
	int value;
	struct bt_node *lchild;
	struct bt_node *rchild;
};

struct bt_ops{
	int (*insert)(struct bt_node **root, int key, int value);
	int (*find)(struct bt_node *root, int key, int *value);
	int (*remove)(struct bt_node *root, int key);
};

struct binary_tree{
	struct bt_node *root;
	struct bt_ops *ops;
};

struct binary_tree *bt_create(void);
void bt_destroy(struct binary_tree *bt);

#endif /* _BINARY_TREE_H_ */
