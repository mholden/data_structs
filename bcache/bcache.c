#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>

#include "bcache.h"

uint32_t bl_phys_type(blk_phys_t *bp) {
    return bp->bp_type;
}

void bl_phys_dump(blk_phys_t *bp) {
    printf("bp_type %" PRIu32 " ", bp->bp_type);
}

blk_phys_t *bl_phys(blk_t *b) {
    return b->bl_phys;
}

uint32_t bl_type(blk_t *b) {
    return bl_phys_type(bl_phys(b));
}

void bl_dump(blk_t *b) {
    printf("bl_blkno: %llu bl_refcount: %d%s ", b->bl_blkno, b->bl_refcnt,
            (b->bl_flags & B_DIRTY) ? " B_DIRTY" : "");
    if (b->bl_bco_ops->bco_dump) {
        printf("bl_bco: ");
        b->bl_bco_ops->bco_dump(b->bl_bco);
    }
}

static void bc_dump_locked(bcache_t *bc) {
    blk_t *b;
    blk_list_t *bl;
    printf("block cache @ %p: ", bc);
    printf("bc_blksz: %" PRIu32 " ", bc->bc_blksz);
    printf("bc_currsz: %" PRIu32 " (%" PRIu32 ") ", bc->bc_currsz, bc->bc_currsz / bc->bc_blksz);
    printf("bc_maxsz: %" PRIu32 " (%" PRIu32 ") ", bc->bc_maxsz, bc->bc_maxsz / bc->bc_blksz);
    printf("bcs_hits: %" PRIu64 " ", bc->bc_stats.bcs_hits);
    printf("bcs_misses: %" PRIu64 " ", bc->bc_stats.bcs_misses);
    printf("bcs_writes: %" PRIu64 " ", bc->bc_stats.bcs_writes);
    printf("bcs_flushes: %" PRIu64 " ", bc->bc_stats.bcs_flushes);
    printf("\n");
    for (int i = 0; i < (bc->bc_maxsz / bc->bc_blksz) * 2; i++) {
        bl = &bc->bc_ht[i];
        if (!LIST_EMPTY(bl)) {
            printf("bc_ht[%d]:\n", i);
            LIST_FOREACH(b, bl, bl_ht_link) {
                printf(" ");
                bl_dump(b);
                printf("\n");
            }
        }
    }
    printf("bc_dl: ");
    LIST_FOREACH(b, &bc->bc_dl, bl_dl_link)
        printf(" %llu", b->bl_blkno);
    printf("\n");
    printf("bc_fl: ");
    TAILQ_FOREACH(b, &bc->bc_fl, bl_fl_link)
        printf(" %llu", b->bl_blkno);
    printf("\n");
}

void bc_dump(bcache_t *bc) {
    lock_lock(bc->bc_lock);
    bc_dump_locked(bc);
    lock_unlock(bc->bc_lock);
    return;
}

bcache_t *bc_create(char *path, uint32_t blksz, uint32_t maxsz){
    bcache_t *bc = NULL;
    
    bc = malloc(sizeof(bcache_t));
    if (!bc)
        goto error_out;
    
    memset(bc, 0, sizeof(bcache_t));
    
    bc->bc_lock = lock_create();
    if (!bc->bc_lock)
        goto error_out;
    
    bc->bc_fd = open(path, O_RDWR);
    if (bc->bc_fd < 0) {
        printf("cache_create: couldn't open %s: %s\n", path, strerror(errno));
        goto error_out;
    }
    
    bc->bc_blksz = blksz;
    bc->bc_currsz = 0;
    bc->bc_maxsz = maxsz;
    
    bc->bc_ht = malloc(sizeof(blk_list_t) * ((bc->bc_maxsz / bc->bc_blksz) * 2));
    if (!bc->bc_ht)
        goto error_out;
    
    memset(bc->bc_ht, 0, sizeof(blk_list_t) * ((bc->bc_maxsz / bc->bc_blksz) * 2));
    
    TAILQ_INIT(&bc->bc_fl);
    
    return bc;
    
error_out:
    if (bc) {
        if (bc->bc_lock)
            lock_destroy(bc->bc_lock);
        if (bc->bc_fd)
            close(bc->bc_fd);
        if (bc->bc_ht)
            free(bc->bc_ht);
        free(bc);
    }
    
    return NULL;
}

// TODO: synchronize this
void bc_destroy(bcache_t *bc) {
    blk_t *b, *bnext;
    blk_list_t *bl;
    
    if (!LIST_EMPTY(&bc->bc_dl))
        printf("bc_destroy: WARNING: dirty list not empty\n");
    
    for (int i = 0; i < (bc->bc_maxsz / bc->bc_blksz) * 2; i++) {
        bl = &bc->bc_ht[i];
        if (!LIST_EMPTY(bl)) {
            LIST_FOREACH_SAFE(b, bl, bl_ht_link, bnext) {
                b->bl_bco_ops->bco_destroy(b->bl_bco);
                free(b->bl_bco_ops);
                free(b->bl_phys);
                assert(b->bl_refcnt == 0);
                if (b->bl_flags & B_DIRTY)
                    printf("bc_destroy: bl_blkno %" PRIu64 " destroyed while dirty\n", b->bl_blkno);
                free(b);
            }
        }
    }
    
    free(bc->bc_ht);
    close(bc->bc_fd);
    lock_destroy(bc->bc_lock);
    free(bc);
}

int bc_get(bcache_t *bc, uint64_t blkno, bco_ops_t *bco_ops, void **bco) {
    blk_t *b;
    blk_list_t *bl;
    bool alloced_b = false;
    int err;
    
    lock_lock(bc->bc_lock);
    
    bl = &bc->bc_ht[blkno % ((bc->bc_maxsz / bc->bc_blksz) * 2)];
    LIST_FOREACH(b, bl, bl_ht_link) {
        if (b->bl_blkno == blkno) {
            bc->bc_stats.bcs_hits++;
            goto found;
        }
    }
    
    bc->bc_stats.bcs_misses++;
    
    // it's not in the cache, so we need to read it from disk
    if ((bc->bc_currsz + bc->bc_blksz) > bc->bc_maxsz) {
        //
        // cache is at max capacity. evict a block
        //
        if (TAILQ_EMPTY(&bc->bc_fl)) {
            printf("bc_get: no blocks free\n");
            //bc_dump_locked(bc);
            err = ENOMEM;
            goto error_out;
        }
        b = TAILQ_FIRST(&bc->bc_fl);
        
        // flush it out if it's dirty
        if (b->bl_flags & B_DIRTY) {
            // TODO: drop lock
            if (pwrite(bc->bc_fd, b->bl_phys, bc->bc_blksz, b->bl_blkno * bc->bc_blksz) != bc->bc_blksz) {
                err = EIO;
                goto error_out;
            }
            b->bl_flags &= ~B_DIRTY;
            LIST_REMOVE(b, bl_dl_link);
            bc->bc_stats.bcs_writes++;
        }
        
        // read in the new block
        if (pread(bc->bc_fd, b->bl_phys, bc->bc_blksz, blkno * bc->bc_blksz) != bc->bc_blksz) {
            err = EIO;
            goto error_out;
        }
        
        b->bl_blkno = blkno;
        
        // re-init bco
        b->bl_bco_ops->bco_destroy(b->bl_bco);
        b->bl_bco = NULL;
        
        memcpy(b->bl_bco_ops, bco_ops, sizeof(bco_ops_t));
        
        err = b->bl_bco_ops->bco_init(&b->bl_bco, b);
        if (err)
            goto error_out;
        
        // get it on the right hash list
        LIST_REMOVE(b, bl_ht_link);
        LIST_INSERT_HEAD(bl, b, bl_ht_link);
    } else {
        //
        // cache is not yet at max capacity. allocate a block
        //
        b = malloc(sizeof(blk_t));
        if (!b) {
            printf("bc_get: failed to allocate memory for b\n");
            err = ENOMEM;
            goto error_out;
        }
        alloced_b = true;
        
        memset(b, 0, sizeof(blk_t));
        
        b->bl_rwlock = rwl_create();
        if (!b->bl_rwlock) {
            printf("bc_get: failed to rwl_create bl_rwlock\n");
            err = ENOMEM;
            goto error_out;
        }
        
        b->bl_bco_ops = malloc(sizeof(bco_ops_t));
        if (!b->bl_bco_ops) {
            printf("bc_get: failed to allocate memory for bl_bco_ops\n");
            err = ENOMEM;
            goto error_out;
        }
        memcpy(b->bl_bco_ops, bco_ops, sizeof(bco_ops_t));
        
        b->bl_phys = malloc(bc->bc_blksz);
        if (!b->bl_phys) {
            printf("bc_get: failed to allocate memory for bl_phys\n");
            err = ENOMEM;
            goto error_out;
        }
        
        // TODO: this should loop until all data is read in as long as pread != -1; short reads aren't errors
        // TODO: drop lock
        if (pread(bc->bc_fd, b->bl_phys, bc->bc_blksz, blkno * bc->bc_blksz) != bc->bc_blksz) {
            err = EIO;
            goto error_out;
        }
        
        b->bl_blkno = blkno;
        
        err = b->bl_bco_ops->bco_init(&b->bl_bco, b);
        if (err)
            goto error_out;
        
        LIST_INSERT_HEAD(bl, b, bl_ht_link);
        TAILQ_INSERT_TAIL(&bc->bc_fl, b, bl_fl_link);
        bc->bc_currsz += bc->bc_blksz;
    }
    
found:
    if (b->bl_refcnt == 0) {
        //
        // if it was free, it isn't anymore.
        // take it off the free list
        //
        TAILQ_REMOVE(&bc->bc_fl, b, bl_fl_link);
    }
    b->bl_refcnt++;
    *bco = b->bl_bco;
    lock_unlock(bc->bc_lock);
    
    return 0;
    
error_out:
    if (alloced_b) {
        if (b->bl_rwlock)
            rwl_destroy(b->bl_rwlock);
        if (b->bl_bco_ops)
            free(b->bl_bco_ops);
        if (b->bl_phys)
            free(b->bl_phys);
        free(b);
    }
    
    lock_unlock(bc->bc_lock);
    
    return err;
}

void bc_dirty(bcache_t *bc, blk_t *b) {
    lock_lock(bc->bc_lock);
    if (!(b->bl_flags & B_DIRTY)) {
        b->bl_flags |= B_DIRTY;
        LIST_INSERT_HEAD(&bc->bc_dl, b, bl_dl_link);
    }
    lock_unlock(bc->bc_lock);
    return;
}

void bc_release(bcache_t *bc, blk_t *b) {
    lock_lock(bc->bc_lock);
    assert(b->bl_refcnt > 0);
    b->bl_refcnt--;
    if (b->bl_refcnt == 0)
        TAILQ_INSERT_TAIL(&bc->bc_fl, b, bl_fl_link);
    lock_unlock(bc->bc_lock);
    return;
}

int bc_flush(bcache_t *bc) {
    blk_t *b, *bnext;
    int err;
    
    lock_lock(bc->bc_lock);
    
    LIST_FOREACH_SAFE(b, &bc->bc_dl, bl_dl_link, bnext) {
        assert(b->bl_flags & B_DIRTY);
        if (pwrite(bc->bc_fd, b->bl_phys, bc->bc_blksz, b->bl_blkno * bc->bc_blksz) != bc->bc_blksz) {
            err = EIO;
            goto error_out;
        }
        b->bl_flags &= ~B_DIRTY;
        LIST_REMOVE(b, bl_dl_link);
        bc->bc_stats.bcs_writes++;
    }
    
    // sync everything out
    err = fsync(bc->bc_fd);
    if (err) {
        err = errno;
        goto error_out;
    }
    
    bc->bc_stats.bcs_flushes++;
    
    lock_unlock(bc->bc_lock);
    
    return 0;
    
error_out:
    lock_unlock(bc->bc_lock);
    return err;
}

void bc_check(bcache_t *bc) {
    blk_t *b, *_b;
    blk_list_t *bl;
    uint32_t nblocks = 0;
    bool free, dirty;
    lock_lock(bc->bc_lock);
    for (int i = 0; i < (bc->bc_maxsz / bc->bc_blksz) * 2; i++) {
        bl = &bc->bc_ht[i];
        if (!LIST_EMPTY(bl)) {
            LIST_FOREACH(b, bl, bl_ht_link) {
                free = false;
                dirty = false;
                LIST_FOREACH(_b, &bc->bc_dl, bl_dl_link) {
                    if (_b == b) {
                        assert(!dirty); // make sure there aren't duplicates
                        dirty = true;
                    }
                }
                TAILQ_FOREACH(_b, &bc->bc_fl, bl_fl_link) {
                    if (_b == b) {
                        assert(!free); // make sure there aren't duplicates
                        free = true;
                    }
                }
                assert((free && (b->bl_refcnt == 0)) || (b->bl_refcnt > 0));
                assert((dirty && (b->bl_flags & B_DIRTY)) || !(b->bl_flags & B_DIRTY));
                if (b->bl_bco_ops->bco_check)
                    b->bl_bco_ops->bco_check(b->bl_bco);
                nblocks++;
            }
        }
    }
    assert(bc->bc_currsz == (nblocks * bc->bc_blksz));
    assert(bc->bc_currsz <= bc->bc_maxsz);
    lock_unlock(bc->bc_lock);
    return;
}
