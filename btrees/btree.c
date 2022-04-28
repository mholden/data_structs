#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "btree.h"
#include "fifo.h"

static const char *bt_bp_type_to_string(uint32_t bp_type) {
    switch (bp_type) {
        case BT_PHYS_TYPE_SM:
            return "BT_PHYS_TYPE_SM";
        case BT_PHYS_TYPE_BM:
            return "BT_PHYS_TYPE_BM";
        case BT_PHYS_TYPE_NODE:
            return "BT_PHYS_TYPE_NODE";
        default:
            return "UNKNOWN";
    }
}

//
// space manager-related functions:
//

static blk_t *sm_block(sm_t *sm) {
    return sm->sm_blk;
}

static int sm_bco_init(void **bco, blk_t *b) {
    sm_t **sm, *_sm;
    sm_phys_t *smp = (sm_phys_t *)b->bl_phys;
    int err;
    
    sm = (sm_t **)bco;
    
    _sm = malloc(sizeof(sm_t));
    if (!_sm) {
        err = ENOMEM;
        goto error_out;
    }
    
    memset(_sm, 0, sizeof(sm_t));
    _sm->sm_blk = b;
    _sm->sm_phys = smp;
    
    *sm = _sm;
    
    return 0;
    
error_out:
    return err;
}

static void sm_bco_destroy(void *bco) {
    sm_t *sm = (sm_t *)bco;
    free(sm);
}

static void sm_bco_dump(void *bco) {
    sm_t *sm = (sm_t *)bco;
    sm_phys_t *smp = sm->sm_phys;
    printf("bp_type %s smp_bsz %" PRIu16 " smp_nblocks %" PRIu64 " ", bt_bp_type_to_string(smp->smp_bp.bp_type), smp->smp_bsz, smp->smp_nblocks);
}

static bco_ops_t sm_bco_ops = {
    .bco_init = sm_bco_init,
    .bco_destroy = sm_bco_destroy,
    .bco_dump = sm_bco_dump,
    .bco_check = NULL
};

//
// btree node-related functions:
//

static bool btn_phys_is_leaf(btn_phys_t *btnp) {
    return (btnp->btnp_flags & BTN_PHYS_FLG_IS_LEAF);
}

static blk_t *btn_block(btn_t *btn) {
    return btn->btn_blk;
}

static btn_phys_t *btn_phys(btn_t *btn) {
    return btn->btn_phys;
}

static bool btn_is_leaf(btn_t *btn) {
    return btn_phys_is_leaf(btn_phys(btn));
}

static void btn_dump(btn_t *btn) {
    btn_phys_t *btnp = btn->btn_phys;
    btr_phys_t *btrp;
    
    printf("bp_type %s btnp_flags 0x%" PRIx16 " btnp_nrecords %" PRIu16 " btnp_freespace %" PRIu16 " btnp_records: ",
            bt_bp_type_to_string(btnp->btnp_bp.bp_type), btnp->btnp_flags, btnp->btnp_nrecords, btnp->btnp_freespace);
    
    btrp = (btr_phys_t *)((uint8_t *)btnp + sizeof(btn_phys_t));
    for (int i = 0; i < btnp->btnp_nrecords; i++) {
        printf("{ ");
        btr_phys_dump(btrp);
        if (!btn_phys_is_leaf(btnp))
            printf("btrp_ptr %" PRIu64 " ", *(uint64_t *)((uint8_t *)btrp + sizeof(btr_phys_t)));
        printf("} ");
        btrp = (btr_phys_t *)((uint8_t *)btrp + btr_phys_size(btrp));
    }
}

static int btn_bco_init(void **bco, blk_t *b) {
    btn_t **btn, *_btn;
    btn_phys_t *btnp = (btn_phys_t *)b->bl_phys;
    int err;
    
    btn = (btn_t **)bco;
    
    _btn = malloc(sizeof(btn_t));
    if (!_btn) {
        err = ENOMEM;
        goto error_out;
    }
    
    memset(_btn, 0, sizeof(btn_t));
    _btn->btn_blk = b;
    _btn->btn_phys = btnp;
    
    *btn = _btn;
    
    return 0;
    
error_out:
    return err;
}

static void btn_bco_destroy(void *bco) {
    btn_t *btn = (btn_t *)bco;
    free(btn);
}

static void btn_bco_dump(void *bco) {
    btn_dump((btn_t *)bco);
}

static bco_ops_t btn_bco_ops = {
    .bco_init = btn_bco_init,
    .bco_destroy = btn_bco_destroy,
    .bco_dump = btn_bco_dump,
    .bco_check = NULL
};

static int bt_lock_shared(btree_t *bt) {
    return rwl_lock_shared(bt->bt_rwlock);
}

static int bt_lock_exclusive(btree_t *bt) {
    return rwl_lock_exclusive(bt->bt_rwlock);
}

static int bt_unlock(btree_t *bt) {
    return rwl_unlock(bt->bt_rwlock);
}

int bt_create(const char *path) {
    uint8_t *buf = NULL, *byte, bit, *extra;
    sm_phys_t *smp;
    bm_phys_t *bmp;
    uint16_t nbmblks, maxbmblks;
    uint32_t blks_per_bm, nextra;
    uint64_t *smp_map, blkno;
    btn_phys_t *btnp;
    bt_info_phys_t *btip;
    ssize_t pret;
    int fd = -1, err;
    
    fd = creat(path, S_IRWXU);
    if (fd < 0) {
        err = errno;
        goto error_out;
    }
    
    err = posix_fallocate(fd, 0, BT_START_SIZE);
    if (err)
        goto error_out;
    
    buf = malloc(BT_PHYS_BLKSZ);
    if (!buf) {
        err = ENOMEM;
        goto error_out;
    }
    
    //
    // set up the space manager:
    //
    
    memset(buf, 0, BT_PHYS_BLKSZ);
    
    smp = (sm_phys_t *)buf;
    smp->smp_bp.bp_type = BT_PHYS_TYPE_SM;
    smp->smp_bsz = BT_PHYS_BLKSZ;
    smp->smp_nblocks = BT_START_SIZE / BT_PHYS_BLKSZ;
    
    blks_per_bm = (smp->smp_bsz - sizeof(bm_phys_t)) * 8;
    nbmblks = ROUND_UP(smp->smp_nblocks, blks_per_bm) / blks_per_bm;
    nextra = (nbmblks * blks_per_bm) - smp->smp_nblocks;
    maxbmblks = (smp->smp_bsz - sizeof(sm_phys_t)) / sizeof(uint64_t) - 1;
    
    //printf("bt_create: smp_bsz %" PRIu32 " smp_nblocks %" PRIu64 " blks_per_bm %" PRIu32 " nbmblks %" PRIu16 " (maxbmblks %" PRIu16 ")\n",
    //        smp->smp_bsz, smp->smp_nblocks, blks_per_bm, nbmblks, maxbmblks);
    
    if (nbmblks > maxbmblks) {
        // TODO: support this
        printf("bt_create: smp_ind_map pointer not yet supported\n");
        err = E2BIG;
        goto error_out;
    }
    
    smp_map = (uint64_t *)(buf + sizeof(sm_phys_t));
    blkno = BT_PHYS_BT_OFFSET + 1;
    for (int i = 0; i < nbmblks; i++)
        smp_map[i] = blkno++;
    
    // write it out
    pret = pwrite(fd, buf, BT_PHYS_BLKSZ, BT_PHYS_SM_OFFSET * BT_PHYS_BLKSZ);
    if (pret != BT_PHYS_BLKSZ) {
        err = EIO;
        goto error_out;
    }
    
    // set up the bitmaps:
    
    assert((nbmblks + 2) <= blks_per_bm);
    
    // mark the space manager, bitmap blocks, and btree root as allocated
    bmp = (bm_phys_t *)buf;
    
    memset(bmp, 0, BT_PHYS_BLKSZ);
    bmp->bmp_bp.bp_type = BT_PHYS_TYPE_BM;
    bmp->bmp_nfree = blks_per_bm;
    
    byte = buf + sizeof(bm_phys_t);
    bit = 1 << 7;
    for (int i = 0; i < nbmblks + 2; i++) {
        *byte |= bit;
        bmp->bmp_nfree--;
        bit >>= 1;
        if (bit == 0) {
            byte++;
            bit = 1 << 7;
        }
    }

    // write it out
    blkno = BT_PHYS_BT_OFFSET + 1;
    pret = pwrite(fd, bmp, BT_PHYS_BLKSZ, blkno++ * BT_PHYS_BLKSZ);
    if (pret != BT_PHYS_BLKSZ) {
        err = EIO;
        goto error_out;
    }
    
    // rest of the bitmaps are fully zeroed
    memset(bmp, 0, BT_PHYS_BLKSZ);
    bmp->bmp_bp.bp_type = BT_PHYS_TYPE_BM;
    bmp->bmp_nfree = blks_per_bm;
    for (int i = 1; i < nbmblks - 1; i++) {
        pret = pwrite(fd, bmp, BT_PHYS_BLKSZ, blkno++ * BT_PHYS_BLKSZ);
        if (pret != BT_PHYS_BLKSZ) {
            err = EIO;
            goto error_out;
        }
    }
    
    // except for the last one, which might have extra bits. mark them as allocated
    if (nextra) {
        extra = buf + BT_PHYS_BLKSZ - (nextra / 8);
        if (nextra / 8)
            memset(extra, 0xff, (nextra / 8));
        if (nextra % 8) {
            byte = extra - 1;
            bit = 1;
            for (int i = 0; i < nextra % 8; i++) {
                *byte |= bit;
                bit <<= 1;
            }
        }
        bmp->bmp_nfree -= nextra;
    }
    
    // write it out
    pret = pwrite(fd, bmp, BT_PHYS_BLKSZ, blkno * BT_PHYS_BLKSZ);
    if (pret != BT_PHYS_BLKSZ) {
        err = EIO;
        goto error_out;
    }
    
    //
    // set up the btree root node
    //
    btnp = (btn_phys_t *)buf;
    
    memset(btnp, 0, BT_PHYS_BLKSZ);
    btnp->btnp_bp.bp_type = BT_PHYS_TYPE_NODE;
    btnp->btnp_flags = BTN_PHYS_FLG_IS_ROOT|BTN_PHYS_FLG_IS_LEAF;
    btnp->btnp_freespace = BT_PHYS_BLKSZ - sizeof(btn_phys_t) - sizeof(bt_info_phys_t);
    
    btip = (bt_info_phys_t *)(buf + BT_PHYS_BLKSZ - sizeof(bt_info_phys_t));
    btip->bti_nnodes = 1;
    
    // write it out
    pret = pwrite(fd, btnp, BT_PHYS_BLKSZ, BT_PHYS_BT_OFFSET * BT_PHYS_BLKSZ);
    if (pret != BT_PHYS_BLKSZ) {
        err = EIO;
        goto error_out;
    }
    
    // sync everything out
    err = fsync(fd);
    if (err) {
        err = errno;
        goto error_out;
    }
    
    err = close(fd);
    if (err) {
        err = errno;
        goto error_out;
    }
    
    free(buf);
    
    return 0;
    
error_out:
    if (fd >= 0)
        close(fd);
    if (buf)
        free(buf);
    
    return err;
}

int bt_open(const char *path, bt_ops_t *ops, btree_t **bt) {
    btree_t *_bt = NULL;
    uint8_t *buf = NULL;
    sm_phys_t *smp;
    ssize_t pret;
    int fd = -1, err;
    
    buf = malloc(BT_PHYS_BLKSZ);
    if (!buf) {
        err = ENOMEM;
        goto error_out;
    }
    
    fd = open(path, O_RDONLY);
    if (fd <= 0) {
        err = errno;
        goto error_out;
    }
    
    pret = pread(fd, buf, BT_PHYS_BLKSZ, BT_PHYS_SM_OFFSET * BT_PHYS_BLKSZ);
    if (pret != BT_PHYS_BLKSZ) {
        err = EIO;
        goto error_out;
    }
    
    smp = (sm_phys_t *)buf;
    
    _bt = malloc(sizeof(btree_t));
    if (!_bt) {
        err = ENOMEM;
        goto error_out;
    }
    
    memset(_bt, 0, sizeof(btree_t));
    
    _bt->bt_rwlock = rwl_create();
    if (!_bt->bt_rwlock) {
        err = ENOMEM;
        goto error_out;
    }
    
    _bt->bt_bc = bc_create((char *)path, smp->smp_bsz, smp->smp_nblocks * smp->smp_bsz / 8);
    if (!_bt->bt_bc) {
        err = ENOMEM;
        goto error_out;
    }
    
    _bt->bt_ops = malloc(sizeof(bt_ops_t));
    if (!_bt->bt_ops) {
        err = ENOMEM;
        goto error_out;
    }
    
    memset(_bt->bt_ops, 0, sizeof(bt_ops_t));
    if (ops)
        memcpy(_bt->bt_ops, ops, sizeof(bt_ops_t));
    
    err = bc_get(_bt->bt_bc, BT_PHYS_SM_OFFSET, &sm_bco_ops, (void **)&_bt->bt_sm);
    if (err)
        goto error_out;
    
    err = bc_get(_bt->bt_bc, BT_PHYS_BT_OFFSET, &btn_bco_ops, (void **)&_bt->bt_root);
    if (err)
        goto error_out;
    
    *bt = _bt;
    
    free(buf);
    
    // block cache has an open fd, so close this now
    err = close(fd);
    if (err) {
        err = errno;
        goto error_out;
    }
    
    return 0;
    
error_out:
    if (buf)
        free(buf);
    if (fd >= 0)
        close(fd);
    if (_bt) {
        if (_bt->bt_rwlock)
            rwl_destroy(_bt->bt_rwlock);
        if (_bt->bt_sm)
            bc_release(_bt->bt_bc, sm_block(_bt->bt_sm));
        if (_bt->bt_root)
            bc_release(_bt->bt_bc, btn_block(_bt->bt_root));
        if (_bt->bt_bc)
            bc_destroy(_bt->bt_bc);
        if (_bt->bt_ops)
            free(_bt->bt_ops);
        free(_bt);
    }
    
    return err;
}

int bt_close(btree_t *bt) {
    bcache_t *bc = bt->bt_bc;
    int err;
    
    err = bc_flush(bc);
    if (err)
        goto error_out;
    
    bc_release(bc, btn_block(bt->bt_root));
    bt->bt_root = NULL;
    
    bc_release(bc, sm_block(bt->bt_sm));
    bt->bt_sm = NULL;
    
    bc_destroy(bc);
    
    rwl_destroy(bt->bt_rwlock);
    free(bt->bt_ops);
    free(bt);
    
    return 0;
    
error_out:
    return err;
}

int bt_destroy(const char *path) {
    int err;
    
    err = unlink(path);
    if (err) {
        err = errno;
        goto error_out;
    }
    
    return 0;
    
error_out:
    return err;
}

int _bt_insert(btree_t *bt, btn_t *btn, btr_phys_t *to_insert) {
    bcache_t *bc = bt->bt_bc;
    btn_phys_t *btnp = btn->btn_phys;
    btr_phys_t *btrp, *_btrp;
    uint16_t recsz, tailsz;
    int comp, err;
    
    if (btn_is_leaf(btn)) {
        recsz = btr_phys_size(to_insert);
        if (recsz > btnp->btnp_freespace) {
            // we'll need to split this node
            assert(0);
        }
        //
        // don't need to split. just insert the record into
        // the node's array of records
        //
        btrp = (btr_phys_t *)((uint8_t *)btnp + sizeof(btn_phys_t));
        tailsz = 0;
        for (int i = 0; i < btnp->btnp_nrecords; i++) {
            comp = bt->bt_ops->bto_compare_fn(to_insert, btrp);
            if (comp == 0) {
                err = EEXIST;
                goto error_out;
            }
            if (comp < 0) {
                _btrp = btrp;
                for (int j = i; j < btnp->btnp_nrecords; j++) {
                    tailsz += btr_phys_size(_btrp);
                    _btrp = (btr_phys_t *)((uint8_t *)_btrp + btr_phys_size(_btrp));
                }
                break;
            }
            btrp = (btr_phys_t *)((uint8_t *)btrp + btr_phys_size(btrp));
        }
        //
        // btrp now points to the location in the btree node
        // where this record should be inserted
        //
        if (tailsz) // shift existing records over to make room
            memmove(btrp + recsz, btrp, tailsz);
        
        memcpy(btrp, to_insert, recsz);
        btnp->btnp_nrecords++;
        btnp->btnp_freespace -= recsz;
        bc_dirty(bc, btn_block(btn));
    } else { // index node
        assert(0);
    }
    
    return 0;
    
error_out:
    return err;
}

int bt_insert(btree_t *bt, btr_phys_t *to_insert) {
    int err;
    
    bt_lock_exclusive(bt);
    
    err = _bt_insert(bt, bt->bt_root, to_insert);
    if (err)
        goto error_out;
    
    bt_unlock(bt);
    
    return 0;
    
error_out:
    bt_unlock(bt);
    
    return err;
}

int _bt_find(btree_t *bt, btn_t *btn, btr_phys_t *to_find, btr_phys_t **record) {
    btn_phys_t *btnp = btn_phys(btn);
    btr_phys_t *btrp, *found;
    int comp = -1, err;
    
    if (btn_is_leaf(btn)) {
        btrp = (btr_phys_t *)((uint8_t *)btnp + sizeof(btn_phys_t));
        for (int i = 0; i < btnp->btnp_nrecords; i++) {
            comp = bt->bt_ops->bto_compare_fn(to_find, btrp);
            if (comp == 0)
                break;
            btrp = (btr_phys_t *)((uint8_t *)btrp + btr_phys_size(btrp));
        }
        if (comp) { // didn't find it
            err = ENOENT;
            goto error_out;
        }
        // did find it and btrp points to it
        found = malloc(btr_phys_size(btrp));
        if (!found) {
            err = ENOMEM;
            goto error_out;
        }
        memcpy(found, btrp, btr_phys_size(btrp));
        *record = found;
    } else { // index
        assert(0);
    }
    
    return 0;
    
error_out:
    return err;
}

int bt_find(btree_t *bt, btr_phys_t *to_find, btr_phys_t **record) {
    int err;
    
    bt_lock_shared(bt);
    
    err = _bt_find(bt, bt->bt_root, to_find, record);
    if (err)
        goto error_out;
    
    bt_unlock(bt);
    
    return 0;
    
error_out:
    bt_unlock(bt);
    return err;
}

int bt_update(btree_t *bt, btr_phys_t *to_update) {
    assert(0);
}

int bt_remove(btree_t *bt, btr_phys_t *to_remove) {
    assert(0);
}

static int _bt_iterate_disk(int fd, uint8_t *buf, uint32_t blksz, int (*node_callback)(btn_phys_t *node, void *ctx, bool *stop), void *node_ctx, int (*record_callback)(btr_phys_t *record, void *ctx, bool *stop), void *record_ctx) {
    btn_phys_t *btnp;
    bt_info_phys_t *btip;
    btr_phys_t *btrp;
    uint32_t nnodes;
    uint64_t *blkno, *blknos = NULL;
    fifo_t *fi = NULL;
    ssize_t pret;
    bool *stop = false;
    int err;
    
    // read in the root node to get nnodes
    pret = pread(fd, buf, blksz, BT_PHYS_BT_OFFSET * blksz);
    if (pret != blksz) {
        err = EIO;
        goto error_out;
    }
    
    btnp = (btn_phys_t *)buf;
    if (btnp->btnp_bp.bp_type != BT_PHYS_TYPE_NODE) {
        printf("_bt_iterate_disk: btnp->btnp_bp.bp_type != BT_PHYS_TYPE_NODE\n");
        err = EILSEQ;
        goto error_out;
    }
    
    btip = (bt_info_phys_t *)(buf + blksz - sizeof(bt_info_phys_t));
    nnodes = btip->bti_nnodes;
    
    fi = fi_create(NULL);
    if (!fi) {
        err = ENOMEM;
        goto error_out;
    }
    
    blknos = malloc(nnodes * sizeof(uint64_t));
    if (!blknos) {
        err = ENOMEM;
        goto error_out;
    }
    
    memset(blknos, 0, nnodes * sizeof(uint64_t));
    blknos[0] = BT_PHYS_BT_OFFSET;
    
    err = fi_enq(fi, &blknos[0]);
    if (err)
        goto error_out;
    
    while (!fi_empty(fi)) {
        err = fi_deq(fi, (void **)&blkno);
        if (err)
            goto error_out;
        
        // read in the node
        pret = pread(fd, buf, blksz, *blkno * blksz);
        if (pret != blksz) {
            err = EIO;
            goto error_out;
        }
        
        btnp = (btn_phys_t *)buf;
        if (btnp->btnp_bp.bp_type != BT_PHYS_TYPE_NODE) {
            printf("_bt_iterate_disk: btnp->btnp_bp.bp_type != BT_PHYS_TYPE_NODE\n");
            err = EILSEQ;
            goto error_out;
        }
        
        if (node_callback) {
            err = node_callback(btnp, node_ctx, stop);
            if (err)
                goto error_out;
            if (stop)
                goto out;
        }
        
        if (btn_phys_is_leaf(btnp)) {
            if (record_callback) {
                btrp = (btr_phys_t *)((uint8_t *)btnp + sizeof(btn_phys_t));
                for (int i = 0; i < btnp->btnp_nrecords; i++) {
                    err = record_callback(btrp, record_ctx, stop);
                    if (err)
                        goto error_out;
                    if (stop)
                        goto out;
                    btrp = (btr_phys_t *)((uint8_t *)btrp + btr_phys_size(btrp));
                }
            }
        } else { // index node
            // add all children to fifo
            assert(0);
        }
    }
    
out:
    fi_destroy(fi);
    free(blknos);
    
    return 0;
    
error_out:
    if (fi)
        fi_destroy(fi);
    if (blknos)
        free(blknos);
    
    return err;
}

int bt_iterate(btree_t *bt, int (*node_callback)(btn_phys_t *node, void *ctx, bool *stop), void *node_ctx, int (*record_callback)(btr_phys_t *record, void *ctx, bool *stop), void *record_ctx) {
    assert(0);
}

int bt_iterate_disk(const char *path, int (*node_callback)(btn_phys_t *node, void *ctx, bool *stop), void *node_ctx, int (*record_callback)(btr_phys_t *record, void *ctx, bool *stop), void *record_ctx) {
    uint8_t *buf = NULL;
    sm_phys_t *smp;
    btn_phys_t *btnp;
    uint16_t blksz;
    int fd = -1, err;
    ssize_t pret;
    
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        err = errno;
        goto error_out;
    }
    
    buf = malloc(BT_PHYS_BLKSZ);
    if (!buf) {
        err = ENOMEM;
        goto error_out;
    }
    
    // read in space manager to get the block size
    pret = pread(fd, buf, BT_PHYS_BLKSZ, BT_PHYS_SM_OFFSET * BT_PHYS_BLKSZ);
    if (pret != BT_PHYS_BLKSZ) {
        err = EIO;
        goto error_out;
    }
    
    smp = (sm_phys_t *)buf;
    
    // sanity checks:
    if (smp->smp_bp.bp_type != BT_PHYS_TYPE_SM) {
        printf("bt_iterate_disk: smp->smp_bp.bp_type != BT_PHYS_TYPE_SM\n");
        err = EILSEQ;
        goto error_out;
    }
    
    if (smp->smp_bsz != BT_PHYS_BLKSZ) {
        // TODO: support this
        printf("bt_iterate_disk: smp->smp_bsz != BT_PHYS_BLKSZ\n");
        err = EILSEQ;
        goto error_out;
    }
    
    blksz = smp->smp_bsz;
    err = _bt_iterate_disk(fd, buf, blksz, node_callback, node_ctx, record_callback, record_ctx);
    if (err)
        goto error_out;
    
    free(buf);
    close(fd);
    
    return 0;
    
error_out:
    if (fd >= 0)
        close(fd);
    if (buf)
        free(buf);
    
    return err;
}

void bt_dump(btree_t *bt) {
    bcache_t *bc = bt->bt_bc;
    
    bt_lock_shared(bt);
    
    printf("bt @ %p:\n", bt);
    bc_dump(bc);
    
    bt_unlock(bt);
    
    return;
}

typedef struct bt_ddn_cb_ctx {
    uint16_t blksz;
} bt_ddn_cb_ctx_t;

static int _bt_dump_disk_node_cb(btn_phys_t *btnp, void *ctx, bool *stop) {
    bt_ddn_cb_ctx_t *btdd_ctx = (bt_ddn_cb_ctx_t *)ctx;
    bt_info_phys_t *btip;
    btr_phys_t *btrp;
    uint16_t blksz = btdd_ctx->blksz;
    
    printf(" bp_type %s btnp_flags 0x%" PRIx16 " btnp_nrecords %" PRIu16 " btnp_freespace %" PRIu16 " btnp_records: ",
            bt_bp_type_to_string(btnp->btnp_bp.bp_type), btnp->btnp_flags, btnp->btnp_nrecords, btnp->btnp_freespace);
    
    btrp = (btr_phys_t *)((uint8_t *)btnp + sizeof(btn_phys_t));
    for (int i = 0; i < btnp->btnp_nrecords; i++) {
        printf("{ btrp_ksz %" PRIu16 " btrp_vsz %" PRIu16 " ", btrp->btrp_ksz, btrp->btrp_vsz);
        if (!(btnp->btnp_flags & BTN_PHYS_FLG_IS_LEAF))
            printf("btrp_ptr %" PRIu64 " ", *(uint64_t *)((uint8_t *)btrp + sizeof(btr_phys_t)));
        printf("} ");
        btrp = (btr_phys_t *)((uint8_t *)btrp + btr_phys_size(btrp));
    }
    
    if (btnp->btnp_flags & BTN_PHYS_FLG_IS_ROOT) {
        btip = (bt_info_phys_t *)((uint8_t *)btnp + blksz - sizeof(bt_info_phys_t));
        printf("bti_nnodes %" PRIu32 " ", btip->bti_nnodes);
    }
    
    printf("\n");
    
    return 0;
}

int bt_dump_disk(const char *path) {
    uint8_t *buf = NULL, *buf2 = NULL, *byte;
    sm_phys_t *smp;
    bm_phys_t *bmp;
    btn_phys_t *btnp;
    uint16_t nbmblks, maxbmblks, blksz;
    uint32_t blks_per_bm;
    uint64_t *smp_map;
    bt_ddn_cb_ctx_t btddn_ctx;
    int fd = -1, err;
    ssize_t pret;
    
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        err = errno;
        goto error_out;
    }
    
    buf = malloc(BT_PHYS_BLKSZ);
    if (!buf) {
        err = ENOMEM;
        goto error_out;
    }
    
    buf2 = malloc(BT_PHYS_BLKSZ);
    if (!buf2) {
        err = ENOMEM;
        goto error_out;
    }
    
    printf("btree @ %s\n", path);
    
    //
    // dump space manager and bitmaps:
    //
    
    pret = pread(fd, buf, BT_PHYS_BLKSZ, BT_PHYS_SM_OFFSET * BT_PHYS_BLKSZ);
    if (pret != BT_PHYS_BLKSZ) {
        err = EIO;
        goto error_out;
    }
    
    smp = (sm_phys_t *)buf;
    
    if (smp->smp_bp.bp_type != BT_PHYS_TYPE_SM) {
        printf("bt_dump_disk: smp->smp_bp.bp_type != BT_PHYS_TYPE_SM\n");
        err = EILSEQ;
        goto error_out;
    }
    
    if (smp->smp_bsz != BT_PHYS_BLKSZ) {
        // TODO: support this
        printf("bt_dump_disk: smp->smp_bsz != BT_PHYS_BLKSZ\n");
        err = EILSEQ;
        goto error_out;
    }
    
    printf("sm @ block %u: bp_type %s smp_bsz %" PRIu16 " smp_nblocks %" PRIu64 " smp_map:\n",
            BT_PHYS_SM_OFFSET, bt_bp_type_to_string(smp->smp_bp.bp_type), smp->smp_bsz, smp->smp_nblocks);
    
    blksz = smp->smp_bsz;
    blks_per_bm = (blksz - sizeof(bm_phys_t)) * 8;
    nbmblks = ROUND_UP(smp->smp_nblocks, blks_per_bm) / blks_per_bm;
    maxbmblks = (blksz- sizeof(sm_phys_t)) / sizeof(uint64_t) - 1;
    
    if (nbmblks > maxbmblks) {
        // TODO: support this
        printf("bt_dump_disk: smp_ind_map pointer not yet supported\n");
        err = E2BIG;
        goto error_out;
    }
    
    smp_map = (uint64_t *)(buf + sizeof(sm_phys_t));
    for (int i = 0; i < nbmblks; i++) {
        printf(" smp_map[%d]: %" PRIu64 "\n", i, smp_map[i]);
        pret = pread(fd, buf2, blksz, smp_map[i] * blksz);
        if (pret != blksz) {
            err = EIO;
            goto error_out;
        }
        bmp = (bm_phys_t *)buf2;
        if (bmp->bmp_bp.bp_type != BT_PHYS_TYPE_BM) {
            printf("bt_dump_disk: bmp->bmp_bp.bp_type != BT_PHYS_TYPE_BM\n");
            err = EILSEQ;
            goto error_out;
        }
        printf("  bm @ block %" PRIu64 ": bp_type %s bmp_nfree %" PRIu32 " bmp_bm:", smp_map[i], bt_bp_type_to_string(bmp->bmp_bp.bp_type), bmp->bmp_nfree);
        byte = buf2 + sizeof(bm_phys_t);
        for (int i = 0; i < blks_per_bm; i+=8) {
            if ((i % 256) == 0)
                printf("\n   ");
            printf("%02x", *byte++);
        }
        printf("\n");
    }
    
    //
    // dump btree:
    //
    
    printf("bt @ block %u:\n", BT_PHYS_BT_OFFSET);
    
    memset(&btddn_ctx, 0, sizeof(bt_ddn_cb_ctx_t));
    btddn_ctx.blksz = blksz;
    
    err = _bt_iterate_disk(fd, buf, blksz, _bt_dump_disk_node_cb, &btddn_ctx, NULL, NULL);
    if (err)
        goto error_out;
    
    err = close(fd);
    if (err) {
        err = errno;
        goto error_out;
    }
    
    free(buf);
    free(buf2);
    
    return 0;
    
error_out:
    if (fd >= 0)
        close(fd);
    if (buf)
        free(buf);
    if (buf2)
        free(buf2);
    
    return err;
}

void bt_check(btree_t *bt) {
    assert(0);
}

int bt_check_disk(const char *path) {
    assert(0);
}

void btr_phys_dump(btr_phys_t *btrp) {
    printf("btrp_ksz %" PRIu16 " btrp_vsz %" PRIu16 " ", btrp->btrp_ksz, btrp->btrp_vsz);
}

uint16_t btr_phys_size(btr_phys_t *btrp) {
    return sizeof(btr_phys_t) + btrp->btrp_ksz + btrp->btrp_vsz;
}
