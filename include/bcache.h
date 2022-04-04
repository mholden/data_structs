#ifndef _BLOCK_CACHE_H_
#define _BLOCK_CACHE_H_

#include <stdint.h>
#include <sys/queue.h>

#include "hash_table.h"
#include "linked_list.h"
#include "synch.h"

// block cache (used to cache blocks either from a file or disk)

#define B_DIRTY 0x0001 // block is dirty

typedef struct block {
    rw_lock_t *bl_rwlock; // XXX: do we need / want this?
    uint64_t bl_blkno;
    int bl_refcnt;
    uint32_t bl_flags;
    void *bl_bco; // back pointer to 'block cache object'
    LIST_ENTRY(block) bl_ht_link; // hash table link
    TAILQ_ENTRY(block) bl_fl_link; // free list link
    LIST_ENTRY(block) bl_dl_link; // dirty list link
    void *bl_data;
} block_t;

// 'block cache object' operations
typedef struct bco_ops {
    int (*bco_init)(void **bco, block_t *b);
    void (*bco_destroy)(void *bco);
    void (*bco_dump)(void *bco);
    void (*bco_check)(void *bco);
} bco_ops_t;

LIST_HEAD(block_list, block);
typedef struct block_list block_list_t;

TAILQ_HEAD(block_tailq, block);
typedef struct block_tailq block_tailq_t;

typedef struct bcache {
    lock_t *bc_lock;
    int bc_fd;
    bco_ops_t *bc_ops;
    block_list_t *bc_ht; // hash table
    block_list_t bc_dl; // dirty list
    block_tailq_t bc_fl; // free list
    uint32_t bc_blksz;
    uint32_t bc_currsz;
    uint32_t bc_maxsz;
} bcache_t;

bcache_t *bc_create(char *path, uint32_t blksz, uint32_t maxsz, bco_ops_t *bco_ops);
void bc_destroy(bcache_t *bc);

int bc_get(bcache_t *bc, uint64_t blkno, void **bco);
void bc_dirty(bcache_t *bc, block_t *b);
void bc_release(bcache_t *bc, block_t *b);

void bc_dump(bcache_t *bc);
void bc_check(bcache_t *bc);

#endif /* _BLOCK_CACHE_H_ */
