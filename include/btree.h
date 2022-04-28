#ifndef _BTREE_H_
#define _BTREE_H_

#include <stdint.h>
#include <stdbool.h>

#include "bcache.h"

#define BT_PHYS_BLKSZ       1024

#define BT_PHYS_SM_OFFSET   0
#define BT_PHYS_BT_OFFSET   1

// btree block types:
#define BT_PHYS_TYPE_SM     0 // space manager
#define BT_PHYS_TYPE_BM     1 // bitmap
#define BT_PHYS_TYPE_NODE   2 // btree node

// btree node flags
#define BTN_PHYS_FLG_IS_ROOT   0x0001
#define BTN_PHYS_FLG_IS_LEAF   0x0002

#define BT_START_SIZE     (64 * 1024 * 1024) // 64MB

typedef struct btree btree_t;

// all btree records start with this header
typedef struct
__attribute__((__packed__))
btree_record_phys {
    uint16_t btrp_ksz; // key size
    uint16_t btrp_vsz; // val size
    //uint8_t btrp_key[];
    //uint8_t btrp_val[];
} btr_phys_t;

typedef struct
__attribute__((__packed__))
bt_info_phys {
    uint32_t bti_nnodes;
} bt_info_phys_t;

typedef struct
__attribute__((__packed__))
btree_node_phys {
    blk_phys_t btnp_bp;
    uint16_t btnp_flags;
    uint16_t btnp_nrecords;
    uint16_t btnp_freespace; // in bytes
    //uint8_t btnp_records[];
    //bt_info_phys_t btnp_info; // in root node only
} btn_phys_t;

typedef struct btree_node {
    blk_t *btn_blk;
    btn_phys_t *btn_phys;
} btn_t;

typedef struct
__attribute__((__packed__))
space_manager_phys {
    blk_phys_t smp_bp;
    uint16_t smp_bsz; // block size
    uint64_t smp_nblocks;
    //uint64_t smp_map[]; // pointers to bitmap blocks
    //uint64_t smp_ind_map; // indirect pointer to bitmap blocks (ie. pointer to block of pointers)
} sm_phys_t;

typedef struct space_manager {
    blk_t *sm_blk;
    sm_phys_t *sm_phys;
} sm_t;

typedef struct
__attribute__((__packed__))
bitmap_phys {
    blk_phys_t bmp_bp;
    uint32_t bmp_nfree;
    //uint8_t bmp_bm[]; // bitmap
} bm_phys_t;

typedef struct btree_ops {
    int (*bto_compare_fn)(btr_phys_t *btr1, btr_phys_t *btr2);
    void (*bto_dump_record_fn)(btr_phys_t *btr);
    void (*bto_check_record_fn)(btr_phys_t *btr);
} bt_ops_t;

struct btree {
    rw_lock_t *bt_rwlock;
    bt_ops_t *bt_ops;
    bcache_t *bt_bc;
    btn_t *bt_root;
    sm_t *bt_sm;
};

int bt_create(const char *path);
int bt_open(const char *path, bt_ops_t *ops, btree_t **bt);
int bt_close(btree_t *bt);
int bt_destroy(const char *path);

int bt_insert(btree_t *bt, btr_phys_t *to_insert);
int bt_find(btree_t *bt, btr_phys_t *to_find, btr_phys_t **record);
int bt_update(btree_t *bt, btr_phys_t *to_update);
int bt_remove(btree_t *bt, btr_phys_t *to_remove);

int bt_iterate(btree_t *bt, int (*node_callback)(btn_phys_t *node, void *ctx, bool *stop), void *node_ctx, int (*record_callback)(btr_phys_t *record, void *ctx, bool *stop), void *record_ctx);
int bt_iterate_disk(const char *path, int (*node_callback)(btn_phys_t *node, void *ctx, bool *stop), void *node_ctx, int (*record_callback)(btr_phys_t *record, void *ctx, bool *stop), void *record_ctx);

void bt_dump(btree_t *bt);
int bt_dump_disk(const char *path);

void bt_check(btree_t *bt);
int bt_check_disk(const char *path);

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#define ROUND_DOWN(N, S) ((N / S) * S)

void btr_phys_dump(btr_phys_t *btrp);
uint16_t btr_phys_size(btr_phys_t *btrp);

#endif // _BTREE_H_
