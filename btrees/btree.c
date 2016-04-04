#include "include/btree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

btree_node_t *btree_node_create(uint32_t key_size, uint32_t val_size) {
	btree_node_t *btn = NULL;
	
	btn = malloc(BTREE_NODE_SIZE);
	if (btn == NULL) {
		printf("malloc() error\n");
		return NULL;
	}
	
	memset(btn, 0, BTREE_NODE_SIZE);
	btn->header.key_size = key_size;
	btn->header.val_size = val_size;
	
	return btn;
}

static int index_from_key_ge(btree_node_t *btn, void *key) {
	int index = 0, recs_left = btn->header.num_records;
	void *curr_key = &btn->data[0] + sizeof(uint64_t);
	
	while (recs_left-- > 0) {
		if (*(int *)key < *(int *)curr_key)
			break;
		
		curr_key += btn->header.key_size + btn->header.val_size + sizeof(uint64_t);
		index++;
	}
	
	return index;
}

static int btree_node_insert_record(btree_node_t *btn, ptr_key_val_ptr_t *pkvp, ptr_key_val_ptr_t **ret) {
	int error = 0;
	
	// TODO: support updates
	
	if (node_full(btn)) { // split the root
		printf("need to split root!\n");
		return ENOTSUP;
	} else { // just insert the kvp
		int ind = index_from_key_ge(btn, pkvp->kvp.key);
		
		// shift the records over to make space
		memmove(_pointer_from_index(btn, ind + 1), _pointer_from_index(btn, ind), (btn->header.num_records - (ind)) * sizeof(key_val_ptr_t) + sizeof(uint64_t));

		// now copy in the pkvp
		void *from, *to;
		size_t sz;
		
		from = pkvp;
		to = _pointer_from_index(btn, ind);
		sz = sizeof(uint64_t);
		memmove(to, from, sz);
		
		from = pkvp->kvp.key;
		to += sz;
		sz = btn->header.key_size;
		memmove(to, from, sz);
		
		from = pkvp->kvp.val;
		to += sz;
		sz = btn->header.val_size;
		memmove(to, from, sz);
		
		from = &pkvp->kvp.ptr;
		to += sz;
		sz = sizeof(uint64_t);
		memmove(to, from, sz);
		
		btn->header.num_records++;
		
		//for (int i = 0; i < btn->header.num_records; i++)
		//	printf("btn->data[%d].key = %d\n", i, *(int *)(&btn->data[0] + sizeof(uint64_t) + i * (btn->header.key_size + btn->header.val_size + sizeof(uint64_t))));
	}
	
	return error;
}

static int _btree_insert(btree_node_t *btn, void *key, void *val, ptr_key_val_ptr_t **ret) {
	return ENOTSUP;
}

int btree_insert(btree_t **bt, void *key, void *val) {
	btree_node_t *btn;
	ptr_key_val_ptr_t *pkvp = NULL, *pkvp2 = NULL;
	uint64_t ptr;
	int ind, error = 0;
	
	btn = *bt;
	ind = index_from_key_ge(btn, key);
	ptr = pointer_from_index(btn, ind);
	
	if (ptr == 0) { // root node is a leaf
		pkvp = malloc(sizeof(ptr_key_val_ptr_t));
		if (pkvp == NULL) {
			error = ENOMEM;
			goto out;
		}
		
		memset(pkvp, 0, sizeof(ptr_key_val_ptr_t));
		pkvp->kvp.key = key;
		pkvp->kvp.val = val;
	} else {
		error = _btree_insert((btree_node_t *)ptr, key, val, &pkvp);
		if (error)
			goto out;
	}

	if (pkvp) { // either root node is a leaf, or there was a split below us
		error = btree_node_insert_record(btn, pkvp, &pkvp2);
		if (error)
			goto out;
		
		if (pkvp2) { // root node split
			assert((btree_t *)pkvp2->ptr == *bt);
			
			*bt = btree_node_create(btn->header.key_size, btn->header.val_size);
			if (*bt == NULL) {
				// undo what we did?
				error = ENOMEM;
				goto out;
			}
			
			btn = *bt;
			error = btree_node_insert_record(btn, pkvp2, NULL);
			if (error)
				goto out;
		}
	}
	
out:
	if (pkvp)
		free(pkvp);
	if (pkvp2)
		free(pkvp2);
	
	return error;
}

void btree_destroy(btree_t *bt) {
	// TODO: implement
	
	return;
}