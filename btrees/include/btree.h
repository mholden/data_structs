/*
 * btree (not b+ tree) implementation.
 */

#ifndef _btree_h_
#define _btree_h_

#include <stdint.h>

/*
 * A btree node looks something like this:
 *
 *  ------------------------------------------
 * | header | p | k | v | p | k | v | ... | p |
 *  ------------------------------------------
 *
 * where p is a pointer, k is a key, v is a value.
 */

#define assert(cond) \
	if (!(cond)) { \
		printf("assertion failed: %s:%d %s\n", __FUNCTION__, __LINE__, #cond); \
		exit(1); \
	}

#define BTREE_NODE_SIZE 68 // header (12 bytes) + 3 records (each record == 16 bytes) + 1 pointer (8 bytes)

#define pointer_from_index(btn, ind) \
	*(uint64_t *)(&btn->data[0] + (ind * (sizeof(uint64_t) + btn->header.key_size + btn->header.val_size)))

#define _pointer_from_index(btn, ind) \
	(&btn->data[0] + ((ind) * (sizeof(uint64_t) + btn->header.key_size + btn->header.val_size)))

#define _key_from_index(btn, ind) \
	(&btn->data[0] + sizeof(uint64_t) + (ind * (btn->header.key_size + btn->header.val_size + sizeof(uint64_t))))

#define node_full(btn) \
	(BTREE_NODE_SIZE  - sizeof(btree_node_header_t)) < (sizeof(uint64_t) + (((btn)->header.num_records + 1) * ((btn->header.key_size + (btn)->header.val_size + sizeof(uint64_t)))))

typedef struct btree_node_header {
	uint32_t num_records;
	uint32_t key_size;
	uint32_t val_size;
} btree_node_header_t;

typedef struct btree_node {
	btree_node_header_t header;
	// records (pointer, {key, val, pointer}s)
	uint8_t data[];
} btree_node_t;

typedef btree_node_t btree_t;

typedef struct key_val_ptr {
	void *key;
	void *val;
	uint64_t ptr;
} key_val_ptr_t;

typedef struct ptr_key_val_ptr {
	uint64_t ptr;
	key_val_ptr_t kvp;
} ptr_key_val_ptr_t;

btree_node_t *btree_node_create(uint32_t key_size, uint32_t val_size);
#define btree_create btree_node_create
int btree_insert(btree_t **bt, void *key, void *val);
int btree_find(btree_t *bt, void *key, void *val);
void btree_destroy(btree_t *bt);

#endif // _btree_h_
