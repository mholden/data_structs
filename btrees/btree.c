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

static void bt_dump_index_record(btree_t *bt, btr_phys_t *btrp) {
    btr_phys_dump(btrp);
    bt->bt_ops->bto_dump_record_fn(btrp, true);
    printf("index_ptr %" PRIu64 " ", btr_phys_index_ptr(btrp));
}

//
// bitmap-related functions:
//

static bm_phys_t *bm_phys(bm_t *bm) {
    return bm->bm_phys;
}

static void bm_dump_phys(bm_t *bm) {
    btree_t *bt = bm->bm_bt;
    sm_t *sm = bt->bt_sm;
    sm_phys_t *smp = sm->sm_phys;
    uint32_t blks_per_bm;
    bm_phys_t *bmp = bm_phys(bm);
    uint8_t *byte;
    
    printf("bp_type %s bmp_nfree %" PRIu32 " ", bt_bp_type_to_string(bmp->bmp_bp.bp_type), bmp->bmp_nfree);
    
#if 0
    printf("bmp_bm: ");
    blks_per_bm = (smp->smp_bsz - sizeof(bm_phys_t)) * 8;
    byte = (uint8_t *)bmp + sizeof(bm_phys_t);
    for (int i = 0; i < blks_per_bm; i+=8) {
        if ((i % 256) == 0)
            printf("\n   ");
        printf("%02x", *byte++);
    }
#endif
}

static blk_t *bm_block(bm_t *bm) {
    return bm->bm_blk;
}

static int bm_bco_init(void **bco, blk_t *b) {
    bm_t **bm, *_bm;
    bm_phys_t *bmp = (bm_phys_t *)b->bl_phys;
    int err;
    
    bm = (bm_t **)bco;
    
    _bm = malloc(sizeof(bm_t));
    if (!_bm) {
        err = ENOMEM;
        goto error_out;
    }
    
    memset(_bm, 0, sizeof(bm_t));
    _bm->bm_blk = b;
    _bm->bm_phys = bmp;
    
    *bm = _bm;
    
    return 0;
    
error_out:
    return err;
}

static void bm_bco_destroy(void *bco) {
    bm_t *bm = (bm_t *)bco;
    free(bm);
}

static void bm_bco_dump(void *bco) {
    bm_t *bm = (bm_t *)bco;
    bm_dump_phys(bm);
}

static void bm_bco_check(void *bco) {
    bm_t *bm = (bm_t *)bco;
    bm_phys_t *bmp = bm_phys(bm);
    uint32_t blks_per_bm, nfree;
    btree_t *bt = bm->bm_bt;
    sm_t *sm = bt->bt_sm;
    sm_phys_t *smp = sm->sm_phys;
    uint8_t *byte, bit;
    
    blks_per_bm = (smp->smp_bsz - sizeof(bm_phys_t)) * 8;
    
    assert(bmp->bmp_bp.bp_type == BT_PHYS_TYPE_BM);
    assert(bmp->bmp_nfree < blks_per_bm);
    
    byte = (uint8_t *)bmp + sizeof(bm_phys_t);
    bit = 1 << 7;
    nfree = 0;
    for (int i = 0; i < blks_per_bm; i++) {
        if (!(bit & *byte))
            nfree++;
        bit >>= 1;
        if (bit == 0) {
            byte++;
            bit = 1 << 7;
        }
    }
    assert(bmp->bmp_nfree == nfree);
}

static bco_ops_t bm_bco_ops = {
    .bco_init = bm_bco_init,
    .bco_destroy = bm_bco_destroy,
    .bco_dump = bm_bco_dump,
    .bco_check = bm_bco_check
};

static int bm_get(btree_t *bt, uint64_t blkno, bm_t **bm) {
    bcache_t *bc = bt->bt_bc;
    bm_t *_bm = NULL;
    int err;
    
    err = bc_get(bc, blkno, &bm_bco_ops, (void **)&_bm);
    if (err)
        goto error_out;
    
    if (bl_type(bm_block(_bm)) != BT_PHYS_TYPE_BM) {
        printf("bm_get: bl_type(bm_block(_bm)) != BT_PHYS_TYPE_BM\n");
        err = EILSEQ;
        goto error_out;
    }
    
    _bm->bm_bt = bt;
    
    *bm = _bm;
    
    return 0;
    
error_out:
    if (_bm)
        bc_release(bc, bm_block(_bm));
        
    return err;
}

int bm_balloc(bm_t *bm, uint32_t *bmind) {
    btree_t *bt = bm->bm_bt;
    bcache_t *bc = bt->bt_bc;
    sm_t *sm = bt->bt_sm;
    sm_phys_t *smp = sm->sm_phys;
    bm_phys_t *bmp = bm_phys(bm);
    uint8_t *byte, bit;
    uint32_t blks_per_bm;
    uint32_t ind;
    int err;
    
    if (!bmp->bmp_nfree) {
        err = ENOENT;
        goto error_out;
    }
    
    blks_per_bm = (smp->smp_bsz - sizeof(bm_phys_t)) * 8;
    
    byte = (uint8_t *)bmp + sizeof(bm_phys_t);
    bit = 1 << 7;
    ind = 0;
    for (int i = 0; i < blks_per_bm; i++) {
        if (!(*byte & bit)) {
            *byte |= bit;
            bmp->bmp_nfree--;
            bc_dirty(bc, bm_block(bm));
            break;
        }
        bit >>= 1;
        if (!bit) {
            *byte++;
            bit = 1 << 7;
        }
        ind++;
    }
    
    *bmind = ind;
    
    return 0;
    
error_out:
    return err;
}


//
// space manager-related functions:
//

static sm_phys_t *sm_phys(sm_t *sm) {
    return sm->sm_phys;
}

static void sm_dump_phys(sm_t *sm) {
    sm_phys_t *smp = sm_phys(sm);
    printf("bp_type %s smp_bsz %" PRIu16 " smp_nblocks %" PRIu64 " smp_rblkno %" PRIu64 " ", bt_bp_type_to_string(smp->smp_bp.bp_type), smp->smp_bsz, smp->smp_nblocks, smp->smp_rblkno);
}

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
    sm_dump_phys(sm);
}

static void sm_bco_check(void *bco) {
    sm_t *sm = (sm_t *)bco;
    sm_phys_t *smp = sm_phys(sm);
    btree_t *bt = sm->sm_bt;
    
    assert(smp->smp_bp.bp_type == BT_PHYS_TYPE_SM);
    assert(smp->smp_rblkno == btn_block(bt->bt_root)->bl_blkno);
}

static bco_ops_t sm_bco_ops = {
    .bco_init = sm_bco_init,
    .bco_destroy = sm_bco_destroy,
    .bco_dump = sm_bco_dump,
    .bco_check = sm_bco_check
};

static int sm_get(btree_t *bt, sm_t **sm) {
    bcache_t *bc = bt->bt_bc;
    sm_t *_sm = NULL;
    int err;
    
    err = bc_get(bc, BT_PHYS_SM_OFFSET, &sm_bco_ops, (void **)&_sm);
    if (err)
        goto error_out;
    
    if (bl_type(sm_block(_sm)) != BT_PHYS_TYPE_SM) {
        printf("sm_get: bl_type(sm_block(_sm)) != BT_PHYS_TYPE_SM\n");
        err = EILSEQ;
        goto error_out;
    }
    
    _sm->sm_bt = bt;
    
    *sm = _sm;
    
    return 0;
    
error_out:
    if (_sm)
        bc_release(bc, sm_block(_sm));
    
    return err;
}

static int sm_balloc(sm_t *sm, uint64_t *blkno) {
    btree_t *bt = sm->sm_bt;
    bcache_t *bc = bt->bt_bc;
    sm_phys_t *smp = sm_phys(sm);
    bm_t *bm = NULL;
    bm_phys_t *bmp;
    uint16_t nbmblks;
    uint32_t blks_per_bm, bmind;
    uint64_t *smp_map, _blkno;
    int err;
    
    blks_per_bm = (smp->smp_bsz - sizeof(bm_phys_t)) * 8;
    nbmblks = ROUND_UP(smp->smp_nblocks, blks_per_bm) / blks_per_bm;
    
    smp_map = (uint64_t *)((uint8_t *)smp + sizeof(sm_phys_t));
    for (int i = 0; i < nbmblks; i++) {
        err = bm_get(bt, smp_map[i], &bm);
        if (err)
            goto error_out;
        
        bmp = bm_phys(bm);
        if (bmp->bmp_nfree) {
            _blkno = blks_per_bm * i;
            break;
        }
        
        bc_release(bc, bm_block(bm));
        bm = NULL;
    }
    
    if (!bm) {
        err = ENOSPC;
        goto error_out;
    }
    
    err = bm_balloc(bm, &bmind);
    if (err)
        goto error_out;
    
    bc_release(bc, bm_block(bm));
    
    *blkno = _blkno + bmind;
    
    return 0;
    
error_out:
    if (bm)
        bc_release(bc, bm_block(bm));
    
    return err;
}

static int sm_bfree(sm_t *sm, uint64_t blkno) {
    assert(0);
}


//
// btree node-related functions:
//

bool btn_phys_is_leaf(btn_phys_t *btnp) {
    return (btnp->btnp_flags & BTN_PHYS_FLG_IS_LEAF);
}

bool btn_phys_is_root(btn_phys_t *btnp) {
    return (btnp->btnp_flags & BTN_PHYS_FLG_IS_ROOT);
}

int btn_phys_iterate_records(btn_phys_t *btnp, int (*callback)(btr_phys_t *btr, void *ctx, bool *stop), void *ctx) {
    btr_phys_t *btrp;
    bool stop = false;
    int err;
    
    btrp = btn_phys_first_record(btnp);
    for (int i = 0; i < btnp->btnp_nrecords; i++) {
        err = callback(btrp, ctx, &stop);
        if (err)
            goto error_out;
        if (stop)
            goto out;
        btrp = btr_phys_next_record(btrp);
    }
    
out:
    return 0;
    
error_out:
    return err;
}

btr_phys_t *btn_phys_first_record(btn_phys_t *btnp) {
    btr_phys_t *btrp;
    
    btrp = (btr_phys_t *)((uint8_t *)btnp + sizeof(btn_phys_t));
    if (!btn_phys_is_leaf(btnp))
        btrp = (btr_phys_t *)((uint8_t *)btrp + sizeof(uint64_t));
    
    return btrp;
}

uint64_t btn_phys_first_index_record_ptr(btn_phys_t *btnp) {
    return *(uint64_t *)((uint8_t *)btn_phys_first_record(btnp) - sizeof(uint64_t));
}

void btn_init_phys(btn_t *btn, uint16_t flags) {
    btree_t *bt = btn->btn_bt;
    sm_t *sm = bt->bt_sm;
    uint16_t blksz = sm_phys(sm)->smp_bsz;
    bcache_t *bc = bt->bt_bc;
    btn_phys_t *btnp = btn_phys(btn);
    
    memset(btnp, 0, blksz);
    btnp->btnp_bp.bp_type = BT_PHYS_TYPE_NODE;
    btnp->btnp_flags = flags;
    btnp->btnp_freespace = blksz - sizeof(btn_phys_t);
    if (flags & BTN_PHYS_FLG_IS_ROOT)
        btnp->btnp_freespace -= sizeof(bt_info_phys_t);
    bc_dirty(bc, btn_block(btn));
}

blk_t *btn_block(btn_t *btn) {
    return btn->btn_blk;
}

btn_phys_t *btn_phys(btn_t *btn) {
    return btn->btn_phys;
}

btr_phys_t *btn_first_record(btn_t *btn) {
    return btn_phys_first_record(btn_phys(btn));
}

uint64_t btn_first_index_record_ptr(btn_t *btn) {
    return btn_phys_first_index_record_ptr(btn_phys(btn));
}

void btn_dump_phys_record(btn_t *btn, btr_phys_t *btrp) {
    btree_t *bt = btn->btn_bt;
    btr_phys_dump(btrp);
    if (btn_is_leaf(btn)) {
        if (bt->bt_ops->bto_dump_record_fn)
            bt->bt_ops->bto_dump_record_fn(btrp, false);
    } else { // index
        if (bt->bt_ops->bto_dump_record_fn)
            bt->bt_ops->bto_dump_record_fn(btrp, true);
        printf("index_ptr %" PRIu64 " ", btr_phys_index_ptr(btrp));
    }
}

void btn_dump_phys(btn_t *btn) {
    btree_t *bt = btn->btn_bt;
    sm_t *sm = bt->bt_sm;
    sm_phys_t *smp = sm_phys(sm);
    uint16_t blksz = smp->smp_bsz;
    btn_phys_t *btnp = btn_phys(btn);
    btr_phys_t *btrp;
    bt_info_phys_t *btip;
    
    printf("bp_type %s %s%s btnp_flags 0x%" PRIx16 " btnp_nrecords %" PRIu16 " btnp_freespace %" PRIu16 " ",
           bt_bp_type_to_string(btnp->btnp_bp.bp_type), btn_phys_is_leaf(btnp) ? "(LEAF)" : "(INDEX)",
           btn_phys_is_root(btnp) ? " (ROOT)" : "", btnp->btnp_flags, btnp->btnp_nrecords, btnp->btnp_freespace);
    
    if (btnp->btnp_flags & BTN_PHYS_FLG_IS_ROOT) {
        btip = (bt_info_phys_t *)((uint8_t *)btnp + blksz - sizeof(bt_info_phys_t));
        printf("bti_nnodes %" PRIu32 " ", btip->bti_nnodes);
    }
    
#if 1
    printf("btn_records:");
    btrp = btn_phys_first_record(btnp);
    if (!btn_is_leaf(btn) && (btnp->btnp_nrecords > 0))
        printf("\n    [-1]: btrp_ptr %" PRIu64 " ", *(uint64_t *)((uint8_t *)btrp - sizeof(uint64_t)));
    for (int i = 0; i < btnp->btnp_nrecords; i++) {
        printf("\n    [%d]: %p ", i, btrp);
        btn_dump_phys_record(btn, btrp);
        //if (btn_is_leaf(btn) && i == 0)
        //    break; // just dump the first recod for leaves
        btrp = btr_phys_next_record(btrp);
    }
#endif
}

bool btn_is_leaf(btn_t *btn) {
    return btn_phys_is_leaf(btn_phys(btn));
}

bool btn_is_root(btn_t *btn) {
    return btn_phys_is_root(btn_phys(btn));
}

static bool bsi_did_split(btn_split_info_t *bsi) {
    return (bsi->bsi_split_index1 != NULL);
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
    btn_t *btn = (btn_t *)bco;
    btn_dump_phys(btn);
}

static void btn_bco_check(void *bco) {
    btn_t *btn = (btn_t *)bco;
    btn_phys_t *btnp = btn_phys(btn);
    btree_t *bt = btn->btn_bt;
    sm_t *sm = bt->bt_sm;
    sm_phys_t *smp = sm_phys(sm);
    uint16_t max_freespace, freespace;
    btr_phys_t *btrp, *rec_space_start, *rec_space_end;
    bt_info_phys_t *bip;
    
    assert(btnp->btnp_bp.bp_type = BT_PHYS_TYPE_NODE);
    assert((btnp->btnp_flags & ~BTN_PHYS_ALLOWABLE_FLAGS) == 0);
    if (!btn_is_leaf(btn))
        assert(btnp->btnp_nrecords > 0);
    
    max_freespace = smp->smp_bsz - sizeof(btn_phys_t);
    if (btn_is_root(btn))
        max_freespace -= sizeof(bt_info_phys_t);
    assert(btnp->btnp_freespace <= max_freespace);
    
    rec_space_start = btn_first_record(btn);
    rec_space_end = (btr_phys_t *)((uint8_t *)btnp + smp->smp_bsz);
    if (btn_is_root(btn))
        rec_space_end = (btr_phys_t *)((uint8_t *)rec_space_end - sizeof(bt_info_phys_t));
    
    freespace = max_freespace;
    btrp = btn_first_record(btn);
    if (!btn_is_leaf(btn))
        freespace -= sizeof(uint64_t); // for first index pointer
    for (int i = 0; i < btnp->btnp_nrecords; i++) {
        assert(btrp >= rec_space_start && btrp < rec_space_end);
        freespace -= btr_phys_size(btrp);
        btrp = btr_phys_next_record(btrp);
    }
    assert(btnp->btnp_freespace == freespace);
}

static bco_ops_t btn_bco_ops = {
    .bco_init = btn_bco_init,
    .bco_destroy = btn_bco_destroy,
    .bco_dump = btn_bco_dump,
    .bco_check = btn_bco_check
};

#define BTNG_FLG_INIT 0x0001 // initialize a brand new node (used in btn_alloc)

static int btn_get(btree_t *bt, uint64_t blkno, uint32_t flags, uint32_t bip_flags, btn_t **btn) {
    bcache_t *bc = bt->bt_bc;
    sm_t *sm = bt->bt_sm;
    btn_t *_btn = NULL;
    int err;
    
    err = bc_get(bc, blkno, &btn_bco_ops, (void **)&_btn);
    if (err)
        goto error_out;
    
    _btn->btn_bt = bt;
    
    if (flags & BTNG_FLG_INIT) {
        btn_init_phys(_btn, bip_flags);
    } else {
        if (bl_type(btn_block(_btn)) != BT_PHYS_TYPE_NODE) {
            printf("btn_get: bl_type(btn_block(_btn)) != BT_PHYS_TYPE_NODE\n");
            err = EILSEQ;
            goto error_out;
        }
    }
    
    *btn = _btn;
    
    return 0;
    
error_out:
    if (_btn)
        bc_release(bc, btn_block(_btn));
    
    return err;
}

int btn_alloc(btree_t *bt, uint32_t bip_flags, btn_t **nbtn) {
    sm_t *sm = bt->bt_sm;
    uint64_t blkno;
    int err;
    
    err = sm_balloc(sm, &blkno);
    if (err)
        goto error_out;
    
    err = btn_get(bt, blkno, BTNG_FLG_INIT, bip_flags, nbtn);
    if (err)
        goto error_out;
    
    return 0;
    
error_out:
    return err;
}

int btn_insert(btn_t *btn, btr_phys_t *to_insert, btn_split_info_t *bsi) {
    btree_t *bt = btn->btn_bt;
    bcache_t *bc = bt->bt_bc;
    btn_phys_t *btnp = btn_phys(btn);
    btr_phys_t *btrp, *_btrp;
    uint16_t recsz, tailsz = 0;
    int comp, err;
    
    if(!btn_is_leaf(btn) && (btnp->btnp_nrecords == 0)) { // first index record is special
        err = btn_insert_first_index_record(btn, 0, to_insert);
        if (err)
            goto error_out;
        goto out;
    }
        
    recsz = btr_phys_size(to_insert);
    if (recsz > btnp->btnp_freespace) { // we need to split
        err = btn_insert_split(btn, to_insert, bsi);
        if (err)
            goto error_out;
        goto out;
    }
    
    // we don't need to split. just insert into our array of records
    btrp = btn_phys_first_record(btnp);
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
                _btrp = btr_phys_next_record(_btrp);
            }
            break;
        }
        btrp = btr_phys_next_record(btrp);
    }
    //
    // btrp now points to the location in the btree node
    // where this record should be inserted
    //
    if (tailsz) // shift existing records over to make room
        memmove((uint8_t *)btrp + recsz, btrp, tailsz);
    
    memcpy(btrp, to_insert, recsz);
    btnp->btnp_nrecords++;
    btnp->btnp_freespace -= recsz;
    bc_dirty(bc, btn_block(btn));
    
out:
    return 0;
    
error_out:
    return err;
}

int btn_insert_first_index_record(btn_t *btn, uint64_t ptr, btr_phys_t *to_insert) {
    btree_t *bt = btn->btn_bt;
    bcache_t *bc = bt->bt_bc;
    btn_phys_t *btnp = btn_phys(btn);
    btr_phys_t *btrp;
    uint16_t recsz;
    int err;
    
    recsz = btr_phys_size(to_insert) + sizeof(uint64_t);
    assert(recsz <= btnp->btnp_freespace);
    assert(!btn_is_leaf(btn) && btnp->btnp_nrecords == 0);
    
    btrp = btn_first_record(btn);
    memcpy((uint8_t *)btrp - sizeof(uint64_t), &ptr, sizeof(uint64_t));
    memcpy(btrp, to_insert, btr_phys_size(to_insert));
    
    btnp->btnp_nrecords++;
    btnp->btnp_freespace -= recsz;
    bc_dirty(bc, btn_block(btn));
    
    return 0;
    
error_out:
    return err;
}

int btn_insert_split(btn_t *btn, btr_phys_t *to_insert, btn_split_info_t *bsi) {
    btree_t *bt = btn->btn_bt;
    sm_t *sm = bt->bt_sm;
    uint16_t blksz = sm_phys(sm)->smp_bsz;
    uint64_t blkno = 0;
    bcache_t *bc =bt->bt_bc;
    btn_t *nbtn = NULL;
    btn_phys_t *btnp = btn_phys(btn), *orig_btnp = NULL, *nbtnp;
    btr_phys_t *split_point = NULL, *insertion_point = NULL, *index_rec, *btrp, *_btrp, *tailrecs;
    uint16_t split_ind = -1, insertion_ind = -1, headsz = 0, tailsz = 0, itailsz, movesz, nrecsz, maxsz, ntailrecs = 0;
    uint32_t ba_flags = 0;
    btn_split_info_t _bsi, rbsi;
    bt_info_phys_t *bip;
    int comp, err;
    
    nrecsz = btr_phys_size(to_insert);
    assert(nrecsz > btnp->btnp_freespace);
    
#if 0
    printf("bt_insert_split (max bt_max_inline_record_size %u) (nrecsz %u):\n", bt_max_inline_record_size(bt), nrecsz);
    printf(" to_insert: ");
    bt->bt_ops->bto_dump_record_fn(to_insert, !btn_is_leaf(btn));
    printf("\n");
    printf(" node: ");
    btn_dump_phys(btn);
    printf("\n");
#endif
    
    memset(&_bsi, 0, sizeof(btn_split_info_t));
    
    //
    // save a copy of our original btnp so we can restore if necessary
    //
    orig_btnp = malloc(blksz);
    if (!orig_btnp) {
        err = ENOMEM;
        goto error_out;
    }
    memcpy(orig_btnp, btnp, blksz);
    
    //
    // allocate and initialize a new block for the new btree node:
    //
    if (btn_is_leaf(btn))
        ba_flags |= BTN_PHYS_FLG_IS_LEAF;
    
    // assert(*bsi.reserved);
    // err = btn_alloc(bt, ba_flags, SMBA_RESERVED, &nbtn);
    // *bsi.reserved--;
    err = btn_alloc(bt, ba_flags, &nbtn);
    if (err)
        goto error_out;
    nbtnp = btn_phys(nbtn);
    
    //
    // find our split and insertion points:
    //
    maxsz = bt_max_inline_record_size(bt);
    itailsz = nrecsz;
    btrp = btn_phys_first_record(btnp);
    for (int i = 0; i < btnp->btnp_nrecords; i++) {
        if (!split_point && (headsz >= maxsz / 2)) {
            split_point = btrp;
            split_ind = i;
            _btrp = btrp;
            for (int j = i; j < btnp->btnp_nrecords; j++) {
                tailsz += btr_phys_size(_btrp);
                _btrp = btr_phys_next_record(_btrp);
            }
            if (!insertion_point) // to_insert will be in the tail
                tailsz += nrecsz;
        }
        if (!insertion_point) {
            comp = bt->bt_ops->bto_compare_fn(to_insert, btrp);
            if (comp == 0) {
                err = EEXIST;
                goto error_out;
            }
            if (comp < 0) {
                insertion_point = btrp;
                insertion_ind = i;
                _btrp = btrp;
                for (int j = i; j < btnp->btnp_nrecords; j++) {
                    itailsz += btr_phys_size(_btrp);
                    _btrp = btr_phys_next_record(_btrp);
                }
                headsz += nrecsz;
                if (!split_point && (headsz >= maxsz / 2)) {
                    split_point = btrp;
                    split_ind = i;
                    tailsz = itailsz;
                }
            }
        }
        if (!split_point)
            headsz += btr_phys_size(btrp);
        btrp = btr_phys_next_record(btrp);
    }
    
    //
    // if btn was root, it won't be anymore. we send the bt_info stored in the root
    // node back up to the caller via btn_split_info so that they can stick it in the
    // new root node
    //
    if (btn_is_root(btn)) {
        bip = (bt_info_phys_t *)((uint8_t *)btnp + blksz - sizeof(bt_info_phys_t));
        memcpy(&_bsi.bsi_bip, bip, sizeof(bt_info_phys_t));
        btnp->btnp_flags &= ~BTN_PHYS_FLG_IS_ROOT;
        btnp->btnp_freespace += sizeof(bt_info_phys_t);
    }
    
    // note: !insertion_point implies to_insert is > all records in btn
    if ((!insertion_point || insertion_point >= split_point) && (tailsz > maxsz)) { // we might need a third node
        assert(nrecsz > (maxsz / 2)); // this must be the case. to_insert is large
        assert((headsz + nrecsz) > maxsz);
        //
        // if we just insert records from insertion point
        // onwards into new node, we might not need a third
        // node. so let's try that
        //
        split_point = insertion_point;
        split_ind = insertion_ind;
        tailsz = itailsz;
    }
    
    tailrecs = split_point;
    if (tailrecs)
        ntailrecs = btnp->btnp_nrecords - split_ind;
    
    //printf(" split_ind %u split point %p insertion_ind %u insertion_point %p ntailrecs %u tailsz %u itailsz %u\n",
    //            split_ind, split_point, insertion_ind, insertion_point, ntailrecs, tailsz, itailsz);
    
    if (tailrecs) {
        // move the tail records to our new node
        movesz = tailsz;
        if (!insertion_point || (insertion_point >= split_point))
            movesz -= nrecsz;
        memcpy(btn_first_record(nbtn), tailrecs, movesz);
        btnp->btnp_nrecords -= ntailrecs;
        nbtnp->btnp_nrecords += ntailrecs;
        btnp->btnp_freespace += movesz;
        nbtnp->btnp_freespace -= movesz;
        if (!btn_is_leaf(nbtn))
            nbtnp->btnp_freespace -= sizeof(uint64_t); // for the first index pointer
        bc_dirty(bc, btn_block(btn));
        bc_dirty(bc, btn_block(nbtn));
        
#if 0
        printf(" node post move: ");
        btn_dump_phys(btn);
        printf("\n");
        printf(" new node: ");
        btn_dump_phys(nbtn);
        printf("\n");
#endif
    }
    
    memset(&rbsi, 0, sizeof(btn_split_info_t));
    if ((insertion_point && insertion_point < split_point) ||
            (tailsz > maxsz && nrecsz <= btnp->btnp_freespace)) { // to_insert goes into our original btree node
        err = btn_insert(btn, to_insert, &rbsi);
        if (err)
            goto error_out;
        assert(!bsi_did_split(&rbsi));
    } else { // to_insert goes into our new btree node
        err = btn_insert(nbtn, to_insert, &rbsi);
        if (err)
            goto error_out;
        if (bsi_did_split(&rbsi)) { // it might have split
            // this should have been the case:
            assert(tailsz > maxsz && nrecsz > btnp->btnp_freespace);
            // shouldn't ever require 3 nodes for this split, and shouldn't be a root split:
            assert(rbsi.bsi_split_index1 && !rbsi.bsi_split_index2 && !rbsi.bsi_bip.bti_nnodes);
            // pass it back up to the caller via bsi_split_index2
            _bsi.bsi_split_index2 = rbsi.bsi_split_index1;
        }
    }
    
#if 0
    printf(" node post insert: ");
    btn_dump_phys(btn);
    printf("\n");
    printf(" new node post insert: ");
    btn_dump_phys(nbtn);
    printf("\n");
#endif
    
    // build the index record to return up to the caller
    index_rec = btn_first_record(nbtn);
    err = btr_phys_build_index_record(index_rec, btn_block(nbtn)->bl_blkno, &_bsi.bsi_split_index1);
    if (err)
        goto error_out;
    
    bc_release(bc, btn_block(nbtn));
    
    *bsi = _bsi;
    
    free(orig_btnp);
    
    return 0;
    
error_out:
    if (orig_btnp) // XXX revert btnp to orig_btnp?
        free(orig_btnp);
    if (blkno)
        sm_bfree(sm, blkno);
    if (nbtn)
        bc_release(bc, btn_block(nbtn));
    
    return err;
}

bt_info_phys_t *btn_root_info(btn_t *btn) {
    btree_t *bt = btn->btn_bt;
    sm_t *sm = bt->bt_sm;
    sm_phys_t *smp = sm_phys(sm);
    btn_phys_t *btnp = btn_phys(btn);
    assert(btn_is_root(btn));
    return (bt_info_phys_t *)((uint8_t *)btnp + smp->smp_bsz - sizeof(bt_info_phys_t));
}


//
// btree record-related functions:
//

void btr_phys_dump(btr_phys_t *btrp) {
    printf("btrp_sz %" PRIu16 " btrp_ksz %" PRIu16 " btrp_vsz %" PRIu16 " ", btr_phys_size(btrp), btrp->btrp_ksz, btrp->btrp_vsz);
}

uint16_t btr_phys_size(btr_phys_t *btrp) {
    return sizeof(btr_phys_t) + btrp->btrp_ksz + btrp->btrp_vsz;
}

uint16_t btr_phys_index_size(btr_phys_t *btrp) {
    return sizeof(btr_phys_t) + btrp->btrp_ksz + sizeof(uint64_t);
}

btr_phys_t *btr_phys_next_record(btr_phys_t *btrp) {
    return (btr_phys_t *)((uint8_t *)btrp + btr_phys_size(btrp));
}

uint64_t btr_phys_index_ptr(btr_phys_t *btrp) {
    return *(uint64_t *)((uint8_t *)btrp + sizeof(btr_phys_t) + btrp->btrp_ksz);
}

int btr_phys_build_index_record(btr_phys_t *btrp, uint64_t blkno, btr_phys_t **index_record) {
    btr_phys_t *_index_record;
    size_t irecsz;
    int err;
    
    irecsz = btr_phys_index_size(btrp);
    _index_record = malloc(irecsz);
    if (!_index_record) {
        err = ENOMEM;
        goto error_out;
    }
    
    memset(_index_record, 0, irecsz);
    _index_record->btrp_ksz = btrp->btrp_ksz;
    _index_record->btrp_vsz = sizeof(uint64_t);
    memcpy((uint8_t *)_index_record + sizeof(btr_phys_t), (uint8_t *)btrp + sizeof(btr_phys_t), btrp->btrp_ksz);
    memcpy((uint8_t *)_index_record + sizeof(btr_phys_t) + _index_record->btrp_ksz, &blkno, sizeof(uint64_t));
    
    *index_record = _index_record;
    
    return 0;
    
error_out:
    return err;
}


//
// btree functions:
//

static int bt_lock_shared(btree_t *bt) {
    return rwl_lock_shared(bt->bt_rwlock);
}

static int bt_lock_exclusive(btree_t *bt) {
    return rwl_lock_exclusive(bt->bt_rwlock);
}

static int bt_unlock(btree_t *bt) {
    return rwl_unlock(bt->bt_rwlock);
}

uint16_t bt_max_inline_record_size(btree_t *bt) {
    sm_t *sm = bt->bt_sm;
    sm_phys_t *smp = sm_phys(sm);
    return smp->smp_bsz - sizeof(btn_phys_t);
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
    smp->smp_rblkno = BT_PHYS_BT_OFFSET;
    
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
    uint64_t rblkno;
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
    rblkno = smp->smp_rblkno;
    
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
    
    err = sm_get(_bt, &_bt->bt_sm);
    if (err)
        goto error_out;
    
    err = btn_get(_bt, rblkno, 0, 0, &_bt->bt_root);
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

int _bt_insert(btree_t *bt, btn_t *btn, btr_phys_t *to_insert, btn_split_info_t *bsi) {
    bcache_t *bc = bt->bt_bc;
    btn_phys_t *btnp = btn_phys(btn);
    btn_t *sbtn = NULL;
    btr_phys_t *btrp;
    uint64_t index_ptr, sblkno;
    btn_t *child = NULL;
    btn_split_info_t cbsi, ibsi, isbsi;
    bt_info_phys_t *bip;
    int comp, err;
    
    if (btn_is_leaf(btn)) {
        err = btn_insert(btn, to_insert, bsi);
        if (err)
            goto error_out;
    } else { // index node
        btrp = btn_first_record(btn);
        index_ptr = btn_first_index_record_ptr(btn);
        for (int i = 0; i < btnp->btnp_nrecords; i++) {
            comp = bt->bt_ops->bto_compare_fn(to_insert, btrp);
            if (comp < 0)
                break;
            index_ptr = btr_phys_index_ptr(btrp);
            btrp = btr_phys_next_record(btrp);
        }
        //
        // i don't think it's possible for index_ptr to be 0 here,
        // even though it is possible for btn_first_index_record_ptr
        // to be 0.
        // TODO: prove this with a test case
        //
        assert(index_ptr);
        
        // get the child node
        err = btn_get(bt, index_ptr, 0, 0, &child);
        if (err)
            goto error_out;
        
        // recurse
        memset(&cbsi, 0, sizeof(btn_split_info_t));
        err = _bt_insert(bt, child, to_insert, &cbsi);
        if (err)
            goto error_out;
        
        if (bsi_did_split(&cbsi)) { // our child split
            bip = btn_root_info(bt->bt_root);
            bip->bti_nnodes++;
            
            // insert the index passed up to us from our child
            memset(&ibsi, 0, sizeof(btn_split_info_t));
            err = btn_insert(btn, cbsi.bsi_split_index1, &ibsi);
            assert(!err);
            
            if (cbsi.bsi_split_index2) {
                //
                // child split into 3 nodes, so we have another index to insert
                //
                assert(!cbsi.bsi_split_index2); // TODO: support this. the below code should work?
                bip->bti_nnodes++;
                if (bsi_did_split(&ibsi)) {
                    //
                    // we split above when we inserted the first record, so now we
                    // have multiple nodes and need to determine which node to insert
                    // this second record into
                    //
                    comp = bt->bt_ops->bto_compare_fn(cbsi.bsi_split_index2, ibsi.bsi_split_index1);
                    if (comp >= 0) { // we need to insert into one of our split nodes
                        if (ibsi.bsi_split_index2) {
                            comp = bt->bt_ops->bto_compare_fn(cbsi.bsi_split_index2, ibsi.bsi_split_index2);
                            if (comp < 0)
                                sblkno = btr_phys_index_ptr(ibsi.bsi_split_index2);
                            else
                                sblkno = btr_phys_index_ptr(ibsi.bsi_split_index1);
                        } else {
                            sblkno = btr_phys_index_ptr(ibsi.bsi_split_index1);
                        }
                        err = btn_get(bt, sblkno, 0, 0, &sbtn);
                        assert(!err);
                    }
                }
                
                memset(&isbsi, 0, sizeof(btn_split_info_t));
                err = btn_insert(sbtn ? sbtn : btn, cbsi.bsi_split_index2, &isbsi);
                assert(!err);
                
                assert(!bsi_did_split(&isbsi)); // can this happen?
                
                if (sbtn)
                    bc_release(bc, btn_block(sbtn));
            }
            
            *bsi = ibsi;
            
            bc_dirty(bc, btn_block(bt->bt_root));
        }
        
        bc_release(bc, btn_block(child));
        child = NULL;
    }
    
out:
    return 0;
    
error_out:
    if (child)
        bc_release(bc, btn_block(child));
    
    return err;
}

int bt_insert(btree_t *bt, btr_phys_t *to_insert) {
    bcache_t *bc = bt->bt_bc;
    sm_t *sm = bt->bt_sm;
    sm_phys_t *smp = sm_phys(sm);
    btn_split_info_t bsi1, bsi2, *bsi;
    btn_t *rbtn;
    btn_phys_t *rbtnp;
    bt_info_phys_t *btip;
    uint64_t rblkno_old;
    int err;
    
    bt_lock_exclusive(bt);
    
    if (btr_phys_index_size(to_insert) > (bt_max_inline_record_size(bt) - sizeof(bt_info_phys_t) - sizeof(uint64_t))) {
        // this key is too large to fit in an index node (namely the root index node)
        err = E2BIG;
        goto error_out;
    }
    
    if (btr_phys_size(to_insert) > bt_max_inline_record_size(bt)) {
        // TODO: could support larger records via blocklists
        err = E2BIG;
        goto error_out;
    }
    
    memset(&bsi1, 0, sizeof(btn_split_info_t));
    memset(&bsi2, 0, sizeof(btn_split_info_t));
    
    //
    // err = sm_reserve(sm, bt->bt_nlevels * 2 + 2);
    // if (err)
    //      goto error_out;
    //
    
    // bsi1.reserved = bt->bt_nlevels * 2;
    // bsi2.reserved = 2;
    
    err = _bt_insert(bt, bt->bt_root, to_insert, &bsi1);
    if (err)
        goto error_out;
    
    if (bsi_did_split(&bsi1)) { // root split
        bsi = &bsi1;
did_split:
        bc_release(bc, btn_block(bt->bt_root));
        bt->bt_root = NULL;
        
        rblkno_old = smp->smp_rblkno;
        // err = btn_alloc(bt, BTN_PHYS_FLG_IS_ROOT, SMBA_RESERVED, &bt->bt_root);
        err = btn_alloc(bt, BTN_PHYS_FLG_IS_ROOT, &bt->bt_root);
        //
        // we can't recover from failure here. btn_alloc should never
        // fail here since we've reserved space above for the maximum
        // number of blocks we should need with all levels of the
        // btree splitting
        //
        assert(!err);
        
        rbtn = bt->bt_root;
        rbtnp = btn_phys(rbtn);
        
        smp->smp_rblkno = btn_block(rbtn)->bl_blkno;
        bc_dirty(bc, sm_block(sm));
        
        //
        // _bt_insert should have set bsi_bip up for us in the case of a root split.
        // copy it into our new root node now
        //
        assert(bsi->bsi_bip.bti_nnodes);
        btip = (bt_info_phys_t *)((uint8_t *)rbtnp + smp->smp_bsz - sizeof(bt_info_phys_t));
        memcpy(btip, &bsi->bsi_bip, sizeof(bt_info_phys_t));
        btip->bti_nnodes += 2;
        
        assert(bsi->bsi_split_index1);
        err = btn_insert_first_index_record(rbtn, rblkno_old, bsi->bsi_split_index1);
        assert(!err);
        
        if (bsi->bsi_split_index2) {
            btip->bti_nnodes++;
            err = btn_insert(rbtn, bsi->bsi_split_index2, &bsi2);
            assert(!err);
            if (bsi_did_split(&bsi2)) { // might split again here
                assert(!bsi2.bsi_split_index2); // can't/won't split a third time
                bsi = &bsi2;
                goto did_split;
            }
        }
    }
    
    // clean up:
    
    if (bsi_did_split(&bsi1)) {
        free(bsi1.bsi_split_index1);
        if (bsi1.bsi_split_index2)
            free(bsi1.bsi_split_index2);
    }
    // sm_unreserve(sm, bsi1.bsi_reserved);
    
    if (bsi_did_split(&bsi2)) {
        free(bsi2.bsi_split_index1);
        if (bsi2.bsi_split_index2)
            free(bsi2.bsi_split_index2);
    }
    // sm_unreserve(sm, bsi2.bsi_reserved);
    
    bt_unlock(bt);
    
    return 0;
    
error_out:
    //if (bsi.bsi_reserved)
    //    sm_unreserve(sm, bsi.reserved);
    
    bt_unlock(bt);
    
    return err;
}

int _bt_find(btree_t *bt, btn_t *btn, btr_phys_t *to_find, btr_phys_t **record) {
    bcache_t *bc = bt->bt_bc;
    btn_phys_t *btnp = btn_phys(btn);
    btr_phys_t *btrp, *found;
    btn_t *child;
    uint64_t index_ptr;
    int comp = -1, err;
    
    if (btn_is_leaf(btn)) {
        btrp = btn_first_record(btn);
        for (int i = 0; i < btnp->btnp_nrecords; i++) {
            comp = bt->bt_ops->bto_compare_fn(to_find, btrp);
            if (comp == 0)
                break;
            btrp = btr_phys_next_record(btrp);
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
        btrp = btn_first_record(btn);
        index_ptr = btn_first_index_record_ptr(btn);
        for (int i = 0; i < btnp->btnp_nrecords; i++) {
            comp = bt->bt_ops->bto_compare_fn(to_find, btrp);
            if (comp < 0)
                break;
            index_ptr = btr_phys_index_ptr(btrp);
            btrp = btr_phys_next_record(btrp);
        }
        
        if (!index_ptr) {
            err = ENOENT;
            goto error_out;
        }
        
        // get the child node
        err = btn_get(bt, index_ptr, 0, 0, &child);
        if (err)
            goto error_out;
        
        // recurse
        err = _bt_find(bt, child, to_find, record);
        if (err)
            goto error_out;
        
        bc_release(bc, btn_block(child));
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

static int _bt_iterate_disk(int fd, uint64_t rblkno, uint8_t *buf, uint32_t blksz, int (*node_callback)(btn_phys_t *node, void *ctx, bool *stop), void *node_ctx, int (*record_callback)(btr_phys_t *record, void *ctx, bool *stop), void *record_ctx) {
    btn_phys_t *btnp;
    bt_info_phys_t *btip;
    btr_phys_t *btrp;
    uint32_t nnodes, curr_ind;
    uint64_t *blkno, *blknos = NULL, first_index_ptr;
    fifo_t *fi = NULL;
    ssize_t pret;
    bool stop = false;
    int err;
    
    // read in the root node to get nnodes
    pret = pread(fd, buf, blksz, rblkno * blksz);
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
    curr_ind = 0;
    blknos[curr_ind] = rblkno;
    
    err = fi_enq(fi, &blknos[curr_ind++]);
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
            err = node_callback(btnp, node_ctx, &stop);
            if (err)
                goto error_out;
            if (stop)
                goto out;
        }
        
        if (btn_phys_is_leaf(btnp)) {
            if (record_callback) {
                btrp = btn_phys_first_record(btnp);
                for (int i = 0; i < btnp->btnp_nrecords; i++) {
                    err = record_callback(btrp, record_ctx, &stop);
                    if (err)
                        goto error_out;
                    if (stop)
                        goto out;
                    btrp = btr_phys_next_record(btrp);
                }
            }
        } else { // index node: add all children to fifo
            first_index_ptr = btn_phys_first_index_record_ptr(btnp);
            if (first_index_ptr) { 
                blknos[curr_ind] = first_index_ptr;
                err = fi_enq(fi, (void *)&blknos[curr_ind++]);
                if (err)
                    goto error_out;
            }
            btrp = btn_phys_first_record(btnp);
            for (int i = 0; i < btnp->btnp_nrecords; i++) {
                blknos[curr_ind] = btr_phys_index_ptr(btrp);
                err = fi_enq(fi, (void *)&blknos[curr_ind++]);
                if (err)
                    goto error_out;
                btrp = btr_phys_next_record(btrp);
            }
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
    
    // read in space manager to get the block size and root blkno
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
    
    err = _bt_iterate_disk(fd, smp->smp_rblkno, buf, smp->smp_bsz, node_callback, node_ctx, record_callback, record_ctx);
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

int _bt_dump_cb(blk_t *b, void *ctx, bool *stop) {
    printf(" ");
    switch (bl_type(b)) {
        case BT_PHYS_TYPE_SM:
            sm_dump_phys((sm_t *)b->bl_bco);
            break;
        case BT_PHYS_TYPE_NODE:
            btn_dump_phys((btn_t *)b->bl_bco);
            break;
        case BT_PHYS_TYPE_BM:
            bm_dump_phys((bm_t *)b->bl_bco);
            break;
        default:
            printf("_bt_dump_cb: unsupported type: %s\n", bt_bp_type_to_string(bl_type(b)));
            assert(0);
            break;
    }
    printf("\n");
    return 0;
}

void bt_dump_locked(btree_t *bt) {
    bcache_t *bc = bt->bt_bc;
    printf("bt @ %p: ", bt);
    //printf("max_inline_record_size %" PRIu16 " ", bt_max_inline_record_size(bt));
    printf("\n");
    //bc_iterate(bc, _bt_dump_cb, NULL);
    bc_dump(bc);
}

void bt_dump(btree_t *bt) {
    bt_lock_shared(bt);
    bt_dump_locked(bt);
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
    
    printf("bp_type %s %s%s btnp_flags 0x%" PRIx16 " btnp_nrecords %" PRIu16 " btnp_freespace %" PRIu16 " ",
           bt_bp_type_to_string(btnp->btnp_bp.bp_type), btn_phys_is_leaf(btnp) ? "(LEAF)" : "(INDEX)",
           btn_phys_is_root(btnp) ? " (ROOT)" : "", btnp->btnp_flags, btnp->btnp_nrecords, btnp->btnp_freespace);
    
    if (btnp->btnp_flags & BTN_PHYS_FLG_IS_ROOT) {
        btip = (bt_info_phys_t *)((uint8_t *)btnp + blksz - sizeof(bt_info_phys_t));
        printf("bti_nnodes %" PRIu32 " ", btip->bti_nnodes);
    }
    
#if 0
    printf("btn_records:");
    btrp = btn_phys_first_record(btnp);
    if (!btn_phys_is_leaf(btnp))
        printf("\n    [-1]: btrp_ptr %" PRIu64 " ", *(uint64_t *)((uint8_t *)btrp - sizeof(uint64_t)));
    for (int i = 0; i < btnp->btnp_nrecords; i++) {
        printf("\n    [%d]: ", i);
        btr_phys_dump(btrp);
        if (!btn_phys_is_leaf(btnp))
            printf("btrp_ptr %" PRIu64 " ", *(uint64_t *)((uint8_t *)btrp + sizeof(btr_phys_t) + btrp->btrp_ksz));
        btrp = btr_phys_next_record(btrp);
    }
#endif
    
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
    uint64_t rblkno;
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
    
    printf("sm @ block %u: bp_type %s smp_bsz %" PRIu16 " smp_nblocks %" PRIu64 " smp_rblkno %" PRIu64 " smp_map:\n",
            BT_PHYS_SM_OFFSET, bt_bp_type_to_string(smp->smp_bp.bp_type), smp->smp_bsz, smp->smp_nblocks, smp->smp_rblkno);
    
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
        printf("  bm @ block %" PRIu64 ": bp_type %s bmp_nfree %" PRIu32 " ", smp_map[i], bt_bp_type_to_string(bmp->bmp_bp.bp_type), bmp->bmp_nfree);
#if 0
        printf("bmp_bm:");
        byte = buf2 + sizeof(bm_phys_t);
        for (int i = 0; i < blks_per_bm; i+=8) {
            if ((i % 256) == 0)
                printf("\n   ");
            printf("%02x", *byte++);
        }
#endif
        printf("\n");
    }
    
    //
    // dump btree:
    //
    
    printf("bt @ block %u:\n", smp->smp_rblkno);
    
    memset(&btddn_ctx, 0, sizeof(bt_ddn_cb_ctx_t));
    btddn_ctx.blksz = blksz;
    
    err = _bt_iterate_disk(fd, smp->smp_rblkno, buf, blksz, _bt_dump_disk_node_cb, &btddn_ctx, NULL, NULL);
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
    bcache_t *bc = bt->bt_bc;
    
    bt_lock_shared(bt);
    bc_check(bc);
    bt_unlock(bt);
    
    return;
}

static int sm_phys_check_disk(sm_phys_t *smp) {
    int err;
    
    if (smp->smp_bp.bp_type != BT_PHYS_TYPE_SM) {
        printf("sm_phys_check_disk: smp->smp_bp.bp_type != BT_PHYS_TYPE_SM\n");
        err = EILSEQ;
        goto error_out;
    }
    
    if (smp->smp_bsz != BT_PHYS_BLKSZ) {
        // TODO: support this
        printf("sm_phys_check_disk: smp->smp_bsz != BT_PHYS_BLKSZ\n");
        err = EILSEQ;
        goto error_out;
    }
    
    return 0;
    
error_out:
    return err;
}

static int bm_phys_check_disk(bm_phys_t *bmp) {
    int err;
    
    if (bmp->bmp_bp.bp_type != BT_PHYS_TYPE_BM) {
        printf("bm_phys_check_disk: bmp->bmp_bp.bp_type != BT_PHYS_TYPE_BM\n");
        err = EILSEQ;
        goto error_out;
    }
    
    return 0;
    
error_out:
    return err;
}

static void bt_cd_bm_dump(uint8_t *bm, uint16_t nbmblocks, uint16_t blksz) {
    printf("bt check disk bm:");
    for (int i = 0; i < nbmblocks * blksz; i++) {
        if ((i % 32) == 0)
            printf("\n   ");
        printf("%02x", bm[i]);
    }
    printf("\n");
}

static void bt_cd_bm_set(uint8_t *bm, uint64_t blkno) {
    bm[blkno / 8] |= (uint8_t)(1 << (7 - (blkno % 8)));
}

typedef struct bt_cdn_cb_ctx {
    sm_phys_t *smp;
    uint8_t *bm;
    uint32_t nnodes;
} bt_cdn_cb_ctx_t;

static int _bt_dump_check_node_cb(btn_phys_t *btnp, void *ctx, bool *stop) {
    bt_cdn_cb_ctx_t *btcd_ctx = (bt_cdn_cb_ctx_t *)ctx;
    sm_phys_t *smp = btcd_ctx->smp;
    uint8_t *bm = btcd_ctx->bm;
    uint16_t max_freespace, freespace;
    btr_phys_t *btrp;
    uint64_t index_ptr;
    int err;
    
    if (btnp->btnp_bp.bp_type != BT_PHYS_TYPE_NODE) {
        printf("_bt_dump_check_node_cb: btnp->btnp_bp.bp_type != BT_PHYS_TYPE_NODE\n");
        err = EILSEQ;
        goto error_out;
    }
    
    if (btnp->btnp_flags & ~BTN_PHYS_ALLOWABLE_FLAGS) {
        printf("_bt_dump_check_node_cb: btnp->btnp_flags & ~BTN_PHYS_ALLOWABLE_FLAGS\n");
        err = EILSEQ;
        goto error_out;
    }
    
    if (!btn_phys_is_leaf(btnp)) {
        if (btnp->btnp_nrecords == 0) {
            printf("_bt_dump_check_node_cb: btnp->btnp_nrecords == 0\n");
            err = EILSEQ;
            goto error_out;
        }
    }
    
    // check the freespace in the node:
    
    max_freespace = smp->smp_bsz - sizeof(btn_phys_t);
    if (btn_phys_is_root(btnp))
        max_freespace -= sizeof(bt_info_phys_t);
    
    if (btnp->btnp_freespace > max_freespace) {
        printf("_bt_dump_check_node_cb: btnp->btnp_freespace > max_freespace\n");
        err = EILSEQ;
        goto error_out;
    }
    
    freespace = max_freespace;
    btrp = btn_phys_first_record(btnp);
    if (!btn_phys_is_leaf(btnp))
        freespace -= sizeof(uint64_t); // for first index pointer
    for (int i = 0; i < btnp->btnp_nrecords; i++) {
        freespace -= btr_phys_size(btrp);
        btrp = btr_phys_next_record(btrp);
    }
    
    if (btnp->btnp_freespace != freespace) {
        printf("_bt_dump_check_node_cb: btnp->btnp_freespace != freespace\n");
        err = EILSEQ;
        goto error_out;
    }
    
    if (!btn_phys_is_leaf(btnp)) {
        // mark all children as allocated in our bitmap
        bt_cd_bm_set(bm, btn_phys_first_index_record_ptr(btnp));
        btrp = btn_phys_first_record(btnp);
        for (int i = 0; i < btnp->btnp_nrecords; i++) {
            bt_cd_bm_set(bm, btr_phys_index_ptr(btrp));
            btrp = btr_phys_next_record(btrp);
        }
    }
    
    btcd_ctx->nnodes++;
    
    return 0;
    
error_out:
    return err;
}

int bt_check_disk(const char *path) {
    uint8_t *buf = NULL, *buf2 = NULL, *bm = NULL, *extra, *byte, bit;
    sm_phys_t *smp;
    bm_phys_t *bmp;
    btn_phys_t *btroot;
    uint16_t nbmblks, _nbmblks, blksz;
    uint32_t blks_per_bm, bytes_per_bm, _blks_per_bm, nextra;
    uint64_t *smp_map;
    uint64_t remaining;
    bt_cdn_cb_ctx_t btcdn_ctx;
    bt_info_phys_t *bip;
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
    
    //
    // check space manager:
    //
    
    pret = pread(fd, buf, BT_PHYS_BLKSZ, BT_PHYS_SM_OFFSET * BT_PHYS_BLKSZ);
    if (pret != BT_PHYS_BLKSZ) {
        err = EIO;
        goto error_out;
    }
    
    smp = (sm_phys_t *)buf;
    err = sm_phys_check_disk(smp);
    if (err)
        goto error_out;
    
    blksz = smp->smp_bsz;
    bytes_per_bm = blksz - sizeof(bm_phys_t);
    blks_per_bm = bytes_per_bm * 8;
    nbmblks = ROUND_UP(smp->smp_nblocks, blks_per_bm) / blks_per_bm;
    
    // set up our own bitmap so that we can cross-check the space manager bitmaps on disk
    _blks_per_bm = blksz * 8;
    _nbmblks = ROUND_UP(smp->smp_nblocks, _blks_per_bm) / _blks_per_bm;
    nextra = (_nbmblks * _blks_per_bm) - smp->smp_nblocks;
    
    bm = malloc(_nbmblks * blksz);
    if (!bm) {
        err = ENOMEM;
        goto error_out;
    }
    
    memset(bm, 0, _nbmblks * blksz);
    if (nextra) {
        extra = bm + (_nbmblks * blksz) - (nextra / 8);
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
    }
    
    bt_cd_bm_set(bm, BT_PHYS_SM_OFFSET);
    
    //
    // check the space manager bitmaps:
    //
    
    smp_map = (uint64_t *)((uint8_t *)smp + sizeof(sm_phys_t));
    for (int i = 0; i < nbmblks; i++) {
        pret = pread(fd, buf2, blksz, smp_map[i] * blksz);
        if (pret != blksz) {
            err = EIO;
            goto error_out;
        }
        bmp = (bm_phys_t *)buf2;
        err = bm_phys_check_disk(bmp);
        if (err)
            goto error_out;
        bt_cd_bm_set(bm, smp_map[i]);
    }
    
    //
    // check the btree:
    //
    
    memset(&btcdn_ctx, 0, sizeof(bt_cdn_cb_ctx_t));
    btcdn_ctx.smp = smp;
    btcdn_ctx.bm = bm;
    
    bt_cd_bm_set(bm, smp->smp_rblkno);
    err = _bt_iterate_disk(fd, smp->smp_rblkno, buf2, blksz, _bt_dump_check_node_cb, &btcdn_ctx, NULL, NULL);
    if (err)
        goto error_out;
    
    // read in the root node and verify nnodes, etc. are correct
    pret = pread(fd, buf2, blksz, smp->smp_rblkno * blksz);
    if (pret != blksz) {
        err = EIO;
        goto error_out;
    }
    
    btroot = (btn_phys_t *)buf2;
    bip = (bt_info_phys_t *)((uint8_t *)btroot + smp->smp_bsz - sizeof(bt_info_phys_t));
    
    if (bip->bti_nnodes != btcdn_ctx.nnodes) {
        printf("bt_check_disk: bip->bti_nnodes (%" PRIu32 ") != btcdn_ctx.nnodes (%" PRIu32 ")\n", bip->bti_nnodes, btcdn_ctx.nnodes);
        err = EILSEQ;
        goto error_out;
    }
    
    //
    // cross check the space manager bitmaps:
    //
    
    //bt_cd_bm_dump(bm, _nbmblks, blksz);
    remaining = _nbmblks * blksz;
    byte = bm;
    for (int i = 0; i < nbmblks; i++) {
        pret = pread(fd, buf2, blksz, smp_map[i] * blksz);
        if (pret != blksz) {
            err = EIO;
            goto error_out;
        }
        bmp = (bm_phys_t *)buf2;
        //printf("comparing %u bytes of bitmap %d\n", (remaining < bytes_per_bm) ? remaining : bytes_per_bm, i);
        if (memcmp((uint8_t *)bmp + sizeof(bm_phys_t), byte, (remaining < bytes_per_bm) ? remaining : bytes_per_bm)) {
            printf("bt_check_disk: bm mismatch\n");
            err = EILSEQ;
            goto error_out;
        }
        byte += bytes_per_bm;
        remaining -= bytes_per_bm;
    }
    
    err = close(fd);
    if (err) {
        err = errno;
        goto error_out;
    }
    
    free(buf);
    free(buf2);
    free(bm);
    
    return 0;
    
error_out:
    if (fd >= 0)
        close(fd);
    if (buf)
        free(buf);
    if (buf2)
        free(buf2);
    if (bm)
        free(bm);
    
    return err;
}
