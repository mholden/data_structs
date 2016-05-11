#include "include/btree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

// debug functions

static void dump_node(btree_node_t *btn) {
	printf("\n\n** node %llu **\n", (uint64_t)btn);
	
	printf("num_records; %u\n", btn->header.num_records);
	printf("key_size: %u\n", btn->header.key_size);
	printf("val_size: %u\n", btn->header.val_size);
	
	uint8_t *data = btn->data;
	printf("p[0]: %llu\n", *(uint64_t *)data);
	
	for (int i = 0; i < btn->header.num_records; i++) {
		// key
		data += sizeof(uint64_t);
		printf("k[%d]: ", i);
		for (int j = 0; j < btn->header.key_size; j++)
			printf("%02x ", data[j]);
		printf("\n");
		
		// val
		data += btn->header.key_size;
		printf("v[%d]: ", i);
		for (int j = 0; j < btn->header.val_size; j++)
			printf("%02x ", data[j]);
		printf("\n");
		
		// pointer
		data += btn->header.val_size;
		printf("p[%d]: %llu\n", i, *(uint64_t *)data);
	}
}

// static functions

static int index_from_key_ge(btree_node_t *btn, void *key) {
	int index = 0, recs_left = btn->header.num_records;
	void *curr_key = &btn->data[0] + sizeof(uint64_t);
	
	while (recs_left-- > 0) {
		// TODO: this should be a generic 'key compare' function
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
	
	if (node_full(btn)) { // split the node
		btree_node_t *new;
		
		// debug
		printf("\n\nSPLITTING NODE %llu (inserting key %02x)\n", (uint64_t)btn, ((uint8_t *)pkvp->kvp.key)[0]);
		dump_node(btn);
		
		new = btree_node_create(btn->header.key_size, btn->header.val_size);
		if (new == NULL) {
			error = ENOMEM;
			goto out;
		}
		
		// move (num_records / 2 + num_records % 2) pkvps into the new node one at a time
		
		uint8_t *from, *to;
		size_t sz;
		int ind = 0;
		bool using_new = false, used_new = false;
		
		// first ptr
		if (*(int *)pkvp->kvp.key < *(int *)_key_from_index(btn, ind)) { // TODO: this should be a generic 'key compare' function
			from = (uint8_t *)&pkvp->ptr;
			using_new = true;
		} else
			from = _pointer_from_index(btn, ind);
		
		to = _pointer_from_index(new, 0);
		sz = sizeof(uint64_t);
		memmove(to, from, sz);
		
		// kvps
		int i;
		for (i = 0; i < (btn->header.num_records / 2) + (btn->header.num_records % 2); i++) {
			// key
			from = _key_from_index(btn, ind);
			
			if (!used_new) {
				// TODO: this should be a generic 'key compare' function
				if (*(int *)pkvp->kvp.key < *(int *)_key_from_index(btn, ind)) {
					if (!using_new) {
						// use pkvps first ptr, too
						from = (uint8_t *)&pkvp->ptr;
						to = _pointer_from_index(new, ind);
						sz = sizeof(uint64_t);
						memmove(to, from, sz);
					}
					from = pkvp->kvp.key;
					using_new = true;
				}
			}
			
			to += sizeof(uint64_t);
			sz = btn->header.key_size;
			memmove(to, from, sz);
			
			// val
			if (using_new)
				from = pkvp->kvp.val;
			else
				from += btn->header.key_size;
			to += btn->header.key_size;
			sz = btn->header.val_size;
			memmove(to, from, sz);
			
			// ptr
			if (using_new)
				from = (uint8_t *)&pkvp->kvp.ptr;
			else
				from += btn->header.val_size;
			to += btn->header.val_size;
			sz = sizeof(uint64_t);
			memmove(to, from, sz);
			
			if (using_new) {
				using_new = false;
				used_new = true;
			} else
				ind++;
		}
		
		new->header.num_records = (btn->header.num_records / 2) + (btn->header.num_records % 2);
		
		// debug
		dump_node(new);
		
		// the next pkvp to set up is ret (this is the pkvp that gets pushed up to the next level)
		
		assert(ret);
		*ret = malloc(sizeof(ptr_key_val_ptr_t));
		assert(*ret);
		
		(*ret)->ptr = (uint64_t)new;
		
		// key
		from = _key_from_index(btn, ind);
		
		if (!used_new) {
			// TODO: this should be a generic 'key compare' function
			if (*(int *)pkvp->kvp.key < *(int *)_key_from_index(btn, ind)) {
				// use pkvps first ptr, too
				from = (uint8_t *)&pkvp->ptr;
				to = _pointer_from_index(new, i);
				sz = sizeof(uint64_t);
				memmove(to, from, sz);
				
				from = pkvp->kvp.key;
				using_new = true;
			}
		}
		
		(*ret)->kvp.key = malloc(btn->header.key_size);
		assert((*ret)->kvp.key);
		
		to = (*ret)->kvp.key;
		sz = sizeof(uint64_t);
		memmove(to, from, sz);
		
		// val
		if (using_new)
			from = pkvp->kvp.val;
		else
			from += btn->header.key_size;
		
		(*ret)->kvp.val = malloc(btn->header.val_size);
		assert((*ret)->kvp.val);
		
		to = (*ret)->kvp.val;
		sz = btn->header.val_size;
		memmove(to, from, sz);
		
		(*ret)->kvp.ptr = (uint64_t)btn;
		
		if (!using_new)
			ind++;
		
#if 0
		// debug
		printf("\n*ret:\n");
		printf("ptr: %llu\n", (*ret)->ptr);
		printf("key: ");
		for (i = 0; i < btn->header.key_size; i++)
			printf("%02x ", ((uint8_t *)(*ret)->kvp.key)[i]);
		printf("\n");
		printf("val: ");
		for (i = 0; i < btn->header.val_size; i++)
			printf("%02x ", ((uint8_t *)(*ret)->kvp.val)[i]);
		printf("\n");
		printf("ptr: %llu\n", (*ret)->kvp.ptr);
#endif
		
		/* 
		 * Lastly, move num_records / 2 pkvps into the 'other' new node one at a time.
		 * We reuse our original node here.
		 */
		
		// first ptr
		if (using_new) {
			from = (uint8_t *)&pkvp->kvp.ptr;
			using_new = false;
			used_new = true;
		} else if (!used_new && *(int *)pkvp->kvp.key < *(int *)_key_from_index(btn, ind)) { // TODO: this should be a generic 'key compare' function
			from = (uint8_t *)&pkvp->ptr;
			using_new = true;
		} else
			from = _pointer_from_index(btn, ind);
		
		to = _pointer_from_index(btn, 0);
		sz = sizeof(uint64_t);
		memmove(to, from, sz);
		
		for (i = 0; i < btn->header.num_records / 2; i++) {
			// key
			from = _key_from_index(btn, ind);
			
			if (!used_new) {
				// TODO: this should be a generic 'key compare' function
				if (*(int *)pkvp->kvp.key < *(int *)_key_from_index(btn, ind)) {
					if (!using_new) {
						// use pkvps first ptr, too
						from = (uint8_t *)&pkvp->ptr;
						to = _pointer_from_index(btn, ind);
						sz = sizeof(uint64_t);
						memmove(to, from, sz);
					}
					from = pkvp->kvp.key;
					using_new = true;
				}
			}
			
			to += sizeof(uint64_t);
			sz = btn->header.key_size;
			memmove(to, from, sz);
			
			// val
			if (using_new)
				from = pkvp->kvp.val;
			else
				from += btn->header.key_size;
			to += btn->header.key_size;
			sz = btn->header.val_size;
			memmove(to, from, sz);
			
			// ptr
			if (using_new)
				from = (uint8_t *)&pkvp->kvp.ptr;
			else
				from += btn->header.val_size;
			to += btn->header.val_size;
			sz = sizeof(uint64_t);
			memmove(to, from, sz);
			
			if (using_new) {
				using_new = false;
				used_new = true;
			} else
				ind++;
		}
		
		btn->header.num_records = btn->header.num_records / 2;
		
		// debug
		dump_node(btn);
	} else { // just insert the pkvp
		int ind = index_from_key_ge(btn, pkvp->kvp.key);
		void *from, *to;
		size_t sz;
		
		//debug
		printf("\n\nbefore\n");
		dump_node(btn);
		
		// shift the records over to make space
		
		from = _pointer_from_index(btn, ind);
		to = _pointer_from_index(btn, ind + 1);
		sz = (btn->header.num_records - (ind)) * sizeof(key_val_ptr_t) + sizeof(uint64_t);
		memmove(to, from, sz);
		
		// now copy in the pkvp
		
		from = &pkvp->ptr;
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
		
		// debug
		printf("\n\nafter\n");
		dump_node(btn);
		
		// debug
		//if (pointer_from_index(btn, 1)) {
		//	printf("\n\nind 1 node\n");
		//	dump_node((btree_node_t *)pointer_from_index(btn, 1));
		//}
	}
	
out:
	return error;
}

static int _btree_insert(btree_node_t *btn, void *key, void *val, ptr_key_val_ptr_t **ret) {
	ptr_key_val_ptr_t *pkvp = NULL;
	uint64_t ptr;
	int ind, error = 0;
	
	ind = index_from_key_ge(btn, key);
	ptr = pointer_from_index(btn, ind);
	
	if (ptr == 0) { // leaf node
		pkvp = malloc(sizeof(ptr_key_val_ptr_t));
		if (pkvp == NULL) {
			error = ENOMEM;
			goto out;
		}
		
		memset(pkvp, 0, sizeof(ptr_key_val_ptr_t));
		
		pkvp->kvp.key = malloc(btn->header.key_size);
		if (pkvp->kvp.key == NULL) {
			error = ENOMEM;
			goto out;
		}
		
		memmove(pkvp->kvp.key, key, btn->header.key_size);
		
		pkvp->kvp.val = malloc(btn->header.val_size);
		if (pkvp->kvp.val == NULL) {
			error = ENOMEM;
			goto out;
		}
		
		memmove(pkvp->kvp.val, val, btn->header.val_size);
	} else {
		error = _btree_insert((btree_node_t *)ptr, key, val, &pkvp);
		if (error)
			goto out;
	}
	
	if (pkvp) { // either node is a leaf, or there was a split below us
		error = btree_node_insert_record(btn, pkvp, ret);
		if (error)
			goto out;
	}
	
out:
	if (pkvp) {
		if (pkvp->kvp.key)
			free(pkvp->kvp.key);
		if (pkvp->kvp.val)
			free(pkvp->kvp.val);
		free(pkvp);
	}
	
	return error;
}

// non-static functions (API)

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
		
		pkvp->kvp.key = malloc((*bt)->header.key_size);
		if (pkvp->kvp.key == NULL) {
			error = ENOMEM;
			goto out;
		}
		
		memmove(pkvp->kvp.key, key, (*bt)->header.key_size);
		
		pkvp->kvp.val = malloc((*bt)->header.val_size);
		if (pkvp->kvp.val == NULL) {
			error = ENOMEM;
			goto out;
		}
		
		memmove(pkvp->kvp.val, val, (*bt)->header.val_size);
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
	if (pkvp) {
		if (pkvp->kvp.key)
			free(pkvp->kvp.key);
		if (pkvp->kvp.val)
			free(pkvp->kvp.val);
		free(pkvp);
	}
	if (pkvp2) {
		if (pkvp2->kvp.key)
			free(pkvp2->kvp.key);
		if (pkvp2->kvp.val)
			free(pkvp2->kvp.val);
		free(pkvp2);
	}
	
	return error;
}

void btree_destroy(btree_t *bt) {
	// TODO: implement
	
	return;
}
