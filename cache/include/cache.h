#ifndef _BLOCK_CACHE_H_
#define _BLOCK_CACHE_H_

#include <pthread.h>

#include "hash_table.h"
#include "linked_list.h"

/*
 * A block cache (used to cache blocks either from a file or disk).
 */

typedef struct block {
    pthread_rwlock_t bl_lock;
    uint64_t bl_blkno;
    int bl_refcnt;
    void *bl_data;
} block_t;

typedef struct cache {
    pthread_mutex_t ca_lock;
    int ca_fd;
    struct hash_table *ca_ht;
    struct linked_list *ca_free_list;
    uint32_t ca_blksz;
    uint32_t ca_currsz;
    uint32_t ca_maxsz;
} cache_t;

cache_t *cache_create(char *path, uint32_t blksz, uint32_t maxsz);
block_t *cache_get(cache_t *cache, uint64_t blkno);
void cache_release(block_t *blk);

#endif /* _BLOCK_CACHE_H_ */
