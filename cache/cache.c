#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "cache.h"

static int key_compare(void *key1, void *key2){
    uint64_t _key1, _key2;
    
    _key1 = *(uint64_t *)key1;
    _key2 = *(uint64_t *)key2;
    
    if (_key1 == _key2)
        return 0;
    else
        return (_key2 > _key1) ? 1: -1;
}

block_t *cache_get(cache_t *cache, uint64_t blkno){
    block_t *block = NULL;
    
    pthread_mutex_lock(&cache->ca_lock);
    
    if (!ht_find(cache->ca_ht, &blkno, (void *)block)) // block is in the cache
        goto got_block;
    
    // it's not in the cache, so we need to read it in from disk
    if ((cache->ca_currsz + sizeof(block_t) + cache->ca_blksz) < cache->ca_maxsz) {
        // we're not at max size yet, so allocate some more memory
        block = malloc(sizeof(block_t));
        if (block == NULL) {
            printf("malloc() error\n");
            goto fail;
        }
        
        memset(block, 0, sizeof(block_t));
        pthread_rwlock_init(&block->bl_lock, NULL);
        block->bl_blkno = blkno;
        block->bl_refcnt = 0;
        block->bl_data = malloc(cache->ca_blksz);
        if (block->bl_data == NULL) {
            printf("malloc() error\n");
            goto fail;
        }

	// XXX shouldn't hold ca_lock over the pread.. set a flag on block and use condition variable to synchronize 
        if (pread(cache->ca_fd, block->bl_data, cache->ca_blksz, blkno * cache->ca_blksz) != cache->ca_blksz) {
            printf("pread() error\n");
            goto fail;
        }
    } else { // we're at max size, so we need to evict someone
        printf("cache eviction not yet supported\n");
        goto fail;
    }

got_block:
    if (block->bl_refcnt == 0) // remove it from the free list
        ll_remove(cache->ca_free_list, (void *)&blkno, LL_NO_FREE); // XXX don't do this if you just alloc'd it..
    
    block->bl_refcnt++;
    
    pthread_mutex_unlock(&cache->ca_lock);
    
    return block;

fail:
    printf("something failed and your code doesn't deal with failure yet..\n");
    
    pthread_mutex_unlock(&cache->ca_lock);
    
    return block;
}

cache_t *cache_create(char *path, uint32_t blksz, uint32_t maxsz){
    cache_t *cache = NULL;
    
    cache = malloc(sizeof(cache_t));
    if (cache == NULL) {
        printf("malloc() error\n");
        goto fail;
    }
    
    memset(cache, 0, sizeof(cache_t));
    
    pthread_mutex_init(&cache->ca_lock, NULL);
    cache->ca_fd = open(path, O_RDWR);
    if (cache->ca_fd < 0) {
        printf("open() error\n");
        goto fail;
    }
    
    cache->ca_ht = ht_create(sizeof(uint64_t), sizeof(block_t), key_compare, maxsz / blksz);
    if (cache->ca_ht == NULL) {
        printf("ht_create() error\n");
        goto fail;
    }
    
    cache->ca_blksz = blksz;
    cache->ca_currsz = 0;
    cache->ca_maxsz = maxsz;
    
    return cache;
    
fail:
    if (cache) {
        if (cache->ca_fd)
            close(cache->ca_fd);
        
        if (cache->ca_ht)
            ht_destroy(cache->ca_ht);
        
        free(cache);
    }
    
    return NULL;
}
