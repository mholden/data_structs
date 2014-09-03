/*
 * Linked list definition(s).
 */

struct ll_node{
	int key;
	int value;
	struct ll_node *next;
};

struct ll_ops{
	int (*insert)(struct ll_node **root, int key, int value);
	int (*find)(struct ll_node *root, int key, int *value);
	int (*remove)(struct ll_node *root, int key);
};

struct linked_list{
	struct ll_node *root;
	struct ll_ops *ops;
};

/* Create a linked list with standard ops */
struct linked_list *ll_create(void);

/* Destroy a linked list */
void ll_destroy(struct linked_list *ll);
