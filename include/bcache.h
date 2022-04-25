#ifndef _BLOCK_CACHE_H_
#define _BLOCK_CACHE_H_

#include <stdint.h>
#include <sys/queue.h> // on macOS
#include <bsd/sys/queue.h> // on Linux

#include "synch.h"

// block cache (used to cache blocks either from a file or disk)

typedef struct block block_t;

// 'block cache object' operations
typedef struct bco_ops {
    int (*bco_init)(void **bco, block_t *b);
    void (*bco_destroy)(void *bco);
    void (*bco_dump)(void *bco);
    void (*bco_check)(void *bco);
} bco_ops_t;

//
// blk_phys_t:
//  all on disk blocks start with this header
//
typedef struct
__attribute__((__packed__))
blk_phys {
    uint32_t bp_type;
    // uint8_t bp_data[];
} blk_phys_t;

uint32_t bl_phys_type(blk_phys_t *bp);
void bl_phys_dump(blk_phys_t *bp);

//
// block_t:
//
#define B_DIRTY 0x0001 // block is dirty

struct block {
    rw_lock_t *bl_rwlock;
    uint64_t bl_blkno;
    int bl_refcnt;
    uint32_t bl_flags;
    void *bl_bco; // back pointer to 'block cache object'
    bco_ops_t *bl_bco_ops;
    LIST_ENTRY(block) bl_ht_link; // hash table link
    TAILQ_ENTRY(block) bl_fl_link; // free list link
    LIST_ENTRY(block) bl_dl_link; // dirty list link
    blk_phys_t *bl_phys;
};

blk_phys_t *bl_phys(block_t *b);
uint32_t bl_type(block_t *b);
void bl_dump(block_t *b);

LIST_HEAD(block_list, block);
typedef struct block_list block_list_t;

TAILQ_HEAD(block_tailq, block);
typedef struct block_tailq block_tailq_t;

//
// bcache_t:
//

typedef struct bc_stats {
    uint64_t bcs_hits;
    uint64_t bcs_misses;
    uint64_t bcs_writes;
    uint64_t bcs_flushes;
} bc_stats_t;

typedef struct bcache {
    lock_t *bc_lock;
    int bc_fd;
    block_list_t *bc_ht; // hash table
    block_list_t bc_dl; // dirty list
    block_tailq_t bc_fl; // free list
    uint32_t bc_blksz;
    uint32_t bc_currsz;
    uint32_t bc_maxsz;
    bc_stats_t bc_stats;
} bcache_t;

bcache_t *bc_create(char *path, uint32_t blksz, uint32_t maxsz);
void bc_destroy(bcache_t *bc);

int bc_get(bcache_t *bc, uint64_t blkno, bco_ops_t *bco_ops, void **bco);
void bc_dirty(bcache_t *bc, block_t *b);
void bc_release(bcache_t *bc, block_t *b);

int bc_flush(bcache_t *bc);

void bc_dump(bcache_t *bc);
void bc_check(bcache_t *bc);

#endif /* _BLOCK_CACHE_H_ */
