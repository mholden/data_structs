#include <stdio.h>
#include <getopt.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <inttypes.h>

#include "btree.h"
#include "tbr.h"
#include "tbr0.h"
#include "tbr1.h"

#define TEST_BTREE_DIR "/var/tmp/test_btree"

static int tbt_compare(btr_phys_t *btr1, btr_phys_t *btr2) {
    tbr_phys_t *tbrp1, *tbrp2;
    
    tbrp1 = (tbr_phys_t *)btr1;
    tbrp2 = (tbr_phys_t *)btr2;
    
    return tbr_phys_compare(tbrp1, tbrp2);
}

static void tbt_dump_record(btr_phys_t *btrp, bool key_only) {
    tbr_phys_dump((tbr_phys_t *)btrp, key_only);
}

static bt_ops_t tbt_bt_ops = {
    .bto_compare_fn = tbt_compare,
    .bto_dump_record_fn = tbt_dump_record,
    .bto_check_record_fn = NULL
};

static int _tbt_dump_record_cb(btr_phys_t *btr, void *ctx, bool *stop) {
    tbr_phys_t *tbrp = (tbr_phys_t *)btr;
    printf(" ");
    tbr_phys_dump(tbrp, false);
    printf("\n");
    return 0;
}

static int _tbt_dump_cb(blk_t *b, void *ctx, bool *stop) {
    btn_t *btn;
    if (bl_type(b) == BT_PHYS_TYPE_NODE) {
        btn = (btn_t *)b->bl_bco;
        if (btn_is_leaf(btn))
            btn_phys_iterate_records(btn_phys(btn), _tbt_dump_record_cb, NULL);
    }
    return 0;
}

static void tbt_dump(btree_t *bt) {
    bcache_t *bc = bt->bt_bc;
    printf("tbt records @ bt %p:\n", bt);
    bc_iterate(bc, _tbt_dump_cb, NULL);
}

static void tbt_dump_disk(const char *path) {
    printf("tbt records @ %s:\n", path);
    bt_iterate_disk(path, NULL, NULL, _tbt_dump_record_cb, NULL);
}

static void test_node_splitting_random(void) {
    btree_t *bt;
    tbr0_phys_t **tbr0_recs;
    uint8_t tbr0_kstr[sizeof(uint64_t) + 1], tbr0_vstr[sizeof(uint64_t) + 1];
    tbr1_phys_t *tbr1_recs;
    char *fname, *tname = "test_node_splitting_random";
    uint64_t rndk, rndv;
    size_t n = BT_PHYS_BLKSZ /** 16*/;
    
    printf("%s\n", tname);
    
    assert(fname = malloc(strlen(TEST_BTREE_DIR) + strlen(tname) + 2));
    sprintf(fname, "%s/%s", TEST_BTREE_DIR, tname);
    
    assert(bt_create(fname) == 0);
    
    //bt_dump_disk(fname);
    
    assert(bt_open(fname, &tbt_bt_ops, &bt) == 0);
    
    assert(tbr0_recs = malloc(sizeof(tbr0_phys_t *) * n));
    memset(tbr0_recs, 0, sizeof(tbr0_phys_t *) * n);
    
    assert(tbr1_recs = malloc(sizeof(tbr1_phys_t) * n));
    memset(tbr1_recs, 0, sizeof(tbr1_phys_t) * n);
    
    for (int i = 0; i < n; i++) {
        rndk = (uint64_t)rand() << 32 | rand();
        rndv = (uint64_t)rand() << 32 | rand();
        if (rand() % 2) { // insert a tbr0 record
            // make random key and val based on rndk, rndv
            for (int j = 0; j < sizeof(uint64_t); j++) {
                tbr0_kstr[j] = *((uint8_t *)&rndk + j) % 90 + 33;
                tbr0_vstr[j] = *((uint8_t *)&rndv + j) % 90 + 33;
            }
            tbr0_kstr[sizeof(uint64_t)] = 0;
            tbr0_vstr[sizeof(uint64_t)] = 0;
            // insert it
            //printf("building tbr0 kstr %s vstr %s\n", &tbr0_kstr[0], &tbr0_vstr[0]);
            assert(tbr0_build_record(&tbr0_kstr[0], &tbr0_vstr[0], &tbr0_recs[i]) == 0);
            assert(tbr0_insert(bt, tbr0_recs[i]) == 0);
        } else { // insert a tbr1 record
            //printf("building tbr1 rndk %llu rndv %llu\n", rndk, rndv);
            assert(tbr1_build_record(rndk, rndv, &tbr1_recs[i]) == 0);
            assert(tbr1_insert(bt, &tbr1_recs[i]) == 0);
        }
        //bt_dump(bt);
        //tbt_dump(bt);
    }
    
    // close the btree:
    assert(bt_close(bt) == 0);
    bt = NULL;
    
    bt_dump_disk(fname);
    tbt_dump_disk(fname);
    
    assert(bt_destroy(fname) == 0);
    
    for (int i = 0; i < n; i++)
        free(tbr0_recs[i]);
    free(tbr0_recs);
    free(tbr1_recs);
    free(fname);
}

// just insert two records and make sure you can retrieve them
static void test_specific_case1(void) {
    btree_t *bt;
    tbr0_t *tbr0 = NULL;
    tbr0_phys_t *tbr0p, *search_tbr0p;
    char *tbr0_kstr = "tbr0_kstr", *tbr0_vstr = "tbr0_vstr";
    tbr1_t *tbr1 = NULL;
    tbr1_phys_t tbr1p, search_tbr1p, *found_tbr1p;
    uint64_t tbr1_id, tbr1_data;
    char *fname, *tname = "test_specific_case1";
    
    printf("%s\n", tname);
    
    assert(fname = malloc(strlen(TEST_BTREE_DIR) + strlen(tname) + 2));
    sprintf(fname, "%s/%s", TEST_BTREE_DIR, tname);
    
    assert(bt_create(fname) == 0);
    
    assert(bt_check_disk(fname) == 0);
    //bt_dump_disk(fname);
    
    assert(bt_open(fname, &tbt_bt_ops, &bt) == 0);
    bt_check(bt);
    //bt_dump(bt);
    
    // insert a tbr0 record:
    assert(tbr0_build_record(tbr0_kstr, tbr0_vstr, &tbr0p) == 0);
    assert(tbr0_insert(bt, tbr0p) == 0);
    bt_check(bt);
    //bt_dump(bt);
    
    // check that  you can find it:
    assert(tbr0_build_record(tbr0_kstr, NULL, &search_tbr0p) == 0);
    assert(tbr0_get(bt, search_tbr0p, &tbr0) == 0);
    
    // and verify its integrity:
    assert(strcmp((uint8_t *)tbr0->tbr0_key + sizeof(tbr0_key_phys_t), tbr0_kstr) == 0);
    assert(strcmp((uint8_t *)tbr0->tbr0_val + sizeof(tbr0_val_phys_t), tbr0_vstr) == 0);
    
    tbr0_release(tbr0);
    tbr0 = NULL;
    
    //bt_check(bt);
    //bt_dump(bt);
    
    // insert a tbr1 record:
    tbr1_id = (uint64_t)rand() << 32 | rand();
    tbr1_data = (uint64_t)rand() << 32 | rand();
    assert(tbr1_build_record(tbr1_id, tbr1_data, &tbr1p) == 0);
    assert(tbr1_insert(bt, &tbr1p) == 0);
    bt_check(bt);
    //bt_dump(bt);
    
    // check that  you can find it:
    assert(tbr1_build_record(tbr1_id, 0, &search_tbr1p) == 0);
    assert(tbr1_get(bt, &search_tbr1p, &tbr1) == 0);
    
    // and verify its integrity:
    found_tbr1p = tbr1_phys(tbr1);;
    assert(found_tbr1p->tbr1_key.tbr1_id == tbr1_id);
    assert(found_tbr1p->tbr1_val.tbr1_data == tbr1_data);
    
    tbr1_release(tbr1);
    tbr1 = NULL;
    
    // close the btree:
    assert(bt_close(bt) == 0);
    bt = NULL;
    
    assert(bt_check_disk(fname) == 0);
    //bt_dump_disk(fname);
    //tbt_dump_disk(fname);
    
    // re-open and check that you can find your records:
    assert(bt_open(fname, &tbt_bt_ops, &bt) == 0);
    bt_check(bt);
    //bt_dump(bt);
    
    assert(tbr0_get(bt, search_tbr0p, &tbr0) == 0);
    assert(strcmp((uint8_t *)tbr0->tbr0_key + sizeof(tbr0_key_phys_t), tbr0_kstr) == 0);
    assert(strcmp((uint8_t *)tbr0->tbr0_val + sizeof(tbr0_val_phys_t), tbr0_vstr) == 0);
    
    tbr0_release(tbr0);
    tbr0 = NULL;
    
    
    assert(tbr1_get(bt, &search_tbr1p, &tbr1) == 0);
    found_tbr1p = tbr1_phys(tbr1);
    assert(found_tbr1p->tbr1_key.tbr1_id == tbr1_id);
    assert(found_tbr1p->tbr1_val.tbr1_data == tbr1_data);
    
    tbr_release((tbr_t *)tbr1);
    tbr1 = NULL;
    
    assert(bt_close(bt) == 0);
    bt = NULL;
    
    assert(bt_check_disk(fname) == 0);
    
    assert(bt_destroy(fname) == 0);
    
    free(tbr0p);
    free(search_tbr0p);
    free(fname);
}

//
// insert a record that causes a split such that:
//  insertion_point > split_point (see bt_insert_split)
//  size of record is small enough as to not require 3 nodes on the split
//
static void test_node_splitting_1(void) {
    btree_t *bt;
    tbr1_t *tbr1;
    tbr1_phys_t *tbr1_recs, *tbr1p;
    char *fname, *tname = "test_node_splitting_1";
    uint64_t data;
    size_t n;
    
    printf("%s\n", tname);
    
    assert(fname = malloc(strlen(TEST_BTREE_DIR) + strlen(tname) + 2));
    sprintf(fname, "%s/%s", TEST_BTREE_DIR, tname);
    
    assert(bt_create(fname) == 0);
    assert(bt_check_disk(fname) == 0);
    //bt_dump_disk(fname);
    
    assert(bt_open(fname, &tbt_bt_ops, &bt) == 0);
    bt_check(bt);
    
    n = bt_max_inline_record_size(bt) / sizeof(tbr1_phys_t);
    
    //printf("inserting %llu recs\n", n);
    
    assert(tbr1_recs = malloc(sizeof(tbr1_phys_t) * (n + 1)));
    memset(tbr1_recs, 0, sizeof(tbr1_phys_t) * (n + 1));

    data = 0;
    for (int i = 0; i < n; i++) {
        if (i == (n * 3 / 4)) // skip this one
            data++;
        assert(tbr1_build_record(data, data, &tbr1_recs[data]) == 0);
        assert(tbr1_insert(bt, &tbr1_recs[data]) == 0);
        data++;
        bt_check(bt);
        //bt_dump(bt);
        //tbt_dump(bt);
    }
    
    // one more record will cause a split
    data = n * 3 / 4;
    assert(tbr1_build_record(data, data, &tbr1_recs[data]) == 0);
    assert(tbr1_insert(bt, &tbr1_recs[data]) == 0);
    bt_check(bt);
    
    // make sure you can find everything
    for (int i = 0; i < n + 1; i++) {
        assert(tbr1_build_record(i, -1, &tbr1_recs[i]) == 0);
        assert(tbr1_get(bt, &tbr1_recs[i], &tbr1) == 0);
        tbr1p = tbr1_phys(tbr1);
        assert((tbr1p->tbr1_key.tbr1_id == i) && (tbr1p->tbr1_val.tbr1_data == i));
        tbr1_release(tbr1);
    }
    
    // close the btree:
    assert(bt_close(bt) == 0);
    bt = NULL;
    
    assert(bt_check_disk(fname) == 0);
    //bt_dump_disk(fname);
    //tbt_dump_disk(fname);
    
    assert(bt_destroy(fname) == 0);
    
    free(tbr1_recs);
    free(fname);
}

//
// insert a record that causes a split such that:
//  insertion_point < split_point (see bt_insert_split)
//  size of record is small enough as to not require 3 nodes on the split
//
static void test_node_splitting_2(void) {
    btree_t *bt;
    tbr1_t *tbr1;
    tbr1_phys_t *tbr1_recs, *tbr1p;
    char *fname, *tname = "test_node_splitting_2";
    uint64_t data;
    size_t n;
    
    printf("%s\n", tname);
    
    assert(fname = malloc(strlen(TEST_BTREE_DIR) + strlen(tname) + 2));
    sprintf(fname, "%s/%s", TEST_BTREE_DIR, tname);
    
    assert(bt_create(fname) == 0);
    assert(bt_check_disk(fname) == 0);
    //bt_dump_disk(fname);
    
    assert(bt_open(fname, &tbt_bt_ops, &bt) == 0);
    bt_check(bt);
    
    n = bt_max_inline_record_size(bt) / sizeof(tbr1_phys_t);
    
    //printf("inserting %llu recs\n", n);
    
    assert(tbr1_recs = malloc(sizeof(tbr1_phys_t) * (n + 1)));
    memset(tbr1_recs, 0, sizeof(tbr1_phys_t) * (n + 1));

    data = 0;
    for (int i = 0; i < n; i++) {
        if (i == (n * 1 / 4)) // skip this one
            data++;
        assert(tbr1_build_record(data, data, &tbr1_recs[data]) == 0);
        assert(tbr1_insert(bt, &tbr1_recs[data]) == 0);
        data++;
        bt_check(bt);
        //bt_dump(bt);
        //tbt_dump(bt);
    }
    
    // one more record will cause a split
    data = n * 1 / 4;
    assert(tbr1_build_record(data, data, &tbr1_recs[data]) == 0);
    assert(tbr1_insert(bt, &tbr1_recs[data]) == 0);
    bt_check(bt);
    
    // make sure you can find everything
    for (int i = 0; i < n + 1; i++) {
        assert(tbr1_build_record(i, -1, &tbr1_recs[i]) == 0);
        assert(tbr1_get(bt, &tbr1_recs[i], &tbr1) == 0);
        tbr1p = tbr1_phys(tbr1);
        assert((tbr1p->tbr1_key.tbr1_id == i) && (tbr1p->tbr1_val.tbr1_data == i));
        tbr1_release(tbr1);
    }
    
    // close the btree:
    assert(bt_close(bt) == 0);
    bt = NULL;
    
    assert(bt_check_disk(fname) == 0);
    //bt_dump_disk(fname);
    //tbt_dump_disk(fname);
    
    assert(bt_destroy(fname) == 0);
    
    free(tbr1_recs);
    free(fname);
}

//
// insert a record that causes a split such that:
//  insertion_point > split_point (see bt_insert_split)
//  size of record is large enough as to require 3 nodes on the split,
//  but small enough such that if we adjust split point to == insertion
//  point, we'll only need 2 nodes
//
static void test_node_splitting_3(void) {
    btree_t *bt;
    tbr0_t *tbr0;
    tbr0_phys_t **tbr0_recs, **tbr0_search_recs, *tbr0p;
    uint8_t tbr0_str[9], data, *big_str, *k, *v;
    char *fname, *tname = "test_node_splitting_3";
    size_t recsz, n, strsz;
    
    printf("%s\n", tname);
    
    assert(fname = malloc(strlen(TEST_BTREE_DIR) + strlen(tname) + 2));
    sprintf(fname, "%s/%s", TEST_BTREE_DIR, tname);
    
    assert(bt_create(fname) == 0);
    assert(bt_check_disk(fname) == 0);
    //bt_dump_disk(fname);
    
    assert(bt_open(fname, &tbt_bt_ops, &bt) == 0);
    bt_check(bt);
    
    recsz = sizeof(tbr0_phys_t) + (2 * sizeof(tbr0_str));
    n = bt_max_inline_record_size(bt) / recsz;
    
    //printf("inserting %llu recs\n", n);
    
    assert(tbr0_recs = malloc(sizeof(tbr0_phys_t *) * (n + 1)));
    memset(tbr0_recs, 0, sizeof(tbr0_phys_t *) * (n + 1));

    memset(tbr0_str, 0, sizeof(tbr0_str));
    data = 33;
    for (int i = 0; i < n + 1; i++) {
        if (i == (n * 15 / 16)) { // skip this one
            data++;
            continue;
        }
        memset(tbr0_str, data, sizeof(tbr0_str) - 1);
        assert(tbr0_build_record(tbr0_str, tbr0_str, &tbr0_recs[i]) == 0);
        assert(tbr0_insert(bt, tbr0_recs[i]) == 0);
        data++;
        bt_check(bt);
        //bt_dump(bt);
        //tbt_dump(bt);
    }
    
    // now build a large record
    recsz = bt_max_inline_record_size(bt) * 3 / 4;
    strsz = (recsz - sizeof(tbr0_phys_t)) / 2;
    assert(big_str = malloc(strsz));
    memset(big_str, 0, strsz);
    data = 33 + (n * 15 / 16);
    memset(big_str, data, strsz - 1);
    
    // trigger the split:
    assert(tbr0_build_record(big_str, big_str, &tbr0_recs[n * 15 / 16]) == 0);
    assert(tbr0_insert(bt, tbr0_recs[n * 15 / 16]) == 0);
    bt_check(bt);
    //bt_dump(bt);
    
    // make sure we can find everything
    assert(tbr0_search_recs = malloc(sizeof(tbr0_phys_t *) * (n + 1)));
    memset(tbr0_search_recs, 0, sizeof(tbr0_phys_t *) * (n + 1));
    data = 33;
    for (int i = 0; i < n + 1; i++) {
        memset(tbr0_str, data, sizeof(tbr0_str) - 1);
        if (i == (n * 15 / 16))
            assert(tbr0_build_record(big_str, NULL, &tbr0_search_recs[i]) == 0);
        else
            assert(tbr0_build_record(tbr0_str, NULL, &tbr0_search_recs[i]) == 0);
        assert(tbr0_get(bt, tbr0_search_recs[i], &tbr0) == 0);
        tbr0p = tbr0_phys(tbr0);
        k = (uint8_t *)&tbr0_recs[i]->tbr0_key + sizeof(tbr0_key_phys_t);
        v = k + tbr0_recs[i]->tbr0_key.tbr0_kstrlen + sizeof(tbr0_val_phys_t);
        assert(!strcmp(k, (uint8_t *)tbr0->tbr0_key + sizeof(tbr0_key_phys_t)) &&
                !strcmp(v, (uint8_t *)tbr0->tbr0_val + sizeof(tbr0_val_phys_t)));
        tbr0_release(tbr0);
        data++;
    }
    
    // close the btree:
    assert(bt_close(bt) == 0);
    bt = NULL;
    
    assert(bt_check_disk(fname) == 0);
    //bt_dump_disk(fname);
    //tbt_dump_disk(fname);
    
    assert(bt_destroy(fname) == 0);
    
    free(big_str);
    for (int i = 0; i < n + 1; i++) {
        free(tbr0_search_recs[i]);
        free(tbr0_recs[i]);
    }
    free(tbr0_search_recs);
    free(tbr0_recs);
    free(fname);
}

//
// insert a record that causes a split such that:
//  insertion_point > split_point (see bt_insert_split)
//  size of record is large enough as to require 3 nodes on the split
//  and too large to allow for an adjustment of split point to insertion
//  point to allow using only 2 nodes. ie. we'll need 3 nodes for the
//  split
//
static void test_node_splitting_4(void) {
    btree_t *bt;
    tbr0_t *tbr0;
    tbr0_phys_t **tbr0_recs, **tbr0_search_recs, *tbr0p;
    uint8_t tbr0_str[9], data, *big_str, *k, *v;
    char *fname, *tname = "test_node_splitting_4";
    size_t recsz, n, strsz;
    
    printf("%s\n", tname);
    
    assert(fname = malloc(strlen(TEST_BTREE_DIR) + strlen(tname) + 2));
    sprintf(fname, "%s/%s", TEST_BTREE_DIR, tname);
    
    assert(bt_create(fname) == 0);
    assert(bt_check_disk(fname) == 0);
    //bt_dump_disk(fname);
    
    assert(bt_open(fname, &tbt_bt_ops, &bt) == 0);
    bt_check(bt);
    
    recsz = sizeof(tbr0_phys_t) + (2 * sizeof(tbr0_str));
    n = bt_max_inline_record_size(bt) / recsz;
    
    //printf("inserting %llu recs\n", n);
    
    assert(tbr0_recs = malloc(sizeof(tbr0_phys_t *) * (n + 1)));
    memset(tbr0_recs, 0, sizeof(tbr0_phys_t *) * (n + 1));

    memset(tbr0_str, 0, sizeof(tbr0_str));
    data = 33;
    for (int i = 0; i < n + 1; i++) {
        if (i == (n * 3 / 4)) { // skip this one
            data++;
            continue;
        }
        memset(tbr0_str, data, sizeof(tbr0_str) - 1);
        assert(tbr0_build_record(tbr0_str, tbr0_str, &tbr0_recs[i]) == 0);
        assert(tbr0_insert(bt, tbr0_recs[i]) == 0);
        data++;
        bt_check(bt);
        //bt_dump(bt);
        //tbt_dump(bt);
    }
    
    // now build a large record
    recsz = bt_max_inline_record_size(bt);
    strsz = (recsz - sizeof(tbr0_phys_t)) / 2;
    assert(big_str = malloc(strsz));
    memset(big_str, 0, strsz);
    data = 33 + (n * 3 / 4);
    memset(big_str, data, strsz - 1);
    
    // trigger the split:
    assert(tbr0_build_record(big_str, big_str, &tbr0_recs[n * 3 / 4]) == 0);
    assert(tbr0_insert(bt, tbr0_recs[n * 3 / 4]) == 0);
    bt_check(bt);
    //bt_dump(bt);
    
    // make sure we can find everything
    assert(tbr0_search_recs = malloc(sizeof(tbr0_phys_t *) * (n + 1)));
    memset(tbr0_search_recs, 0, sizeof(tbr0_phys_t *) * (n + 1));
    data = 33;
    for (int i = 0; i < n + 1; i++) {
        memset(tbr0_str, data, sizeof(tbr0_str) - 1);
        if (i == (n * 3 / 4))
            assert(tbr0_build_record(big_str, NULL, &tbr0_search_recs[i]) == 0);
        else
            assert(tbr0_build_record(tbr0_str, NULL, &tbr0_search_recs[i]) == 0);
        assert(tbr0_get(bt, tbr0_search_recs[i], &tbr0) == 0);
        tbr0p = tbr0_phys(tbr0);
        k = (uint8_t *)&tbr0_recs[i]->tbr0_key + sizeof(tbr0_key_phys_t);
        v = k + tbr0_recs[i]->tbr0_key.tbr0_kstrlen + sizeof(tbr0_val_phys_t);
        assert(!strcmp(k, (uint8_t *)tbr0->tbr0_key + sizeof(tbr0_key_phys_t)) &&
                !strcmp(v, (uint8_t *)tbr0->tbr0_val + sizeof(tbr0_val_phys_t)));
        tbr0_release(tbr0);
        data++;
    }
    
    // close the btree:
    assert(bt_close(bt) == 0);
    bt = NULL;
    
    assert(bt_check_disk(fname) == 0);
    //bt_dump_disk(fname);
    //tbt_dump_disk(fname);
    
    assert(bt_destroy(fname) == 0);
    
    free(big_str);
    for (int i = 0; i < n + 1; i++) {
        free(tbr0_search_recs[i]);
        free(tbr0_recs[i]);
    }
    free(tbr0_search_recs);
    free(tbr0_recs);
    free(fname);
}

//
// same as test_node_splitting_4 but make the key really large
// so that it triggers an index-node split at the root node
// post splitting the leaf node
//
static void test_node_splitting_5(void) {
    btree_t *bt;
    tbr0_t *tbr0;
    tbr0_phys_t **tbr0_recs, **tbr0_search_recs, *tbr0p;
    uint8_t tbr0_str[9], data, *big_kstr, *vstr, *k, *v;
    char *fname, *tname = "test_node_splitting_5";
    size_t recsz, n, strsz, ksz, vsz;
    
    printf("%s\n", tname);
    
    assert(fname = malloc(strlen(TEST_BTREE_DIR) + strlen(tname) + 2));
    sprintf(fname, "%s/%s", TEST_BTREE_DIR, tname);
    
    assert(bt_create(fname) == 0);
    assert(bt_check_disk(fname) == 0);
    //bt_dump_disk(fname);
    
    assert(bt_open(fname, &tbt_bt_ops, &bt) == 0);
    bt_check(bt);
    
    recsz = sizeof(tbr0_phys_t) + (2 * sizeof(tbr0_str));
    n = bt_max_inline_record_size(bt) / recsz;
    
    //printf("inserting %llu recs\n", n);
    
    assert(tbr0_recs = malloc(sizeof(tbr0_phys_t *) * (n + 1)));
    memset(tbr0_recs, 0, sizeof(tbr0_phys_t *) * (n + 1));

    memset(tbr0_str, 0, sizeof(tbr0_str));
    data = 33;
    for (int i = 0; i < n + 1; i++) {
        if (i == (n * 3 / 4)) { // skip this one
            data++;
            continue;
        }
        memset(tbr0_str, data, sizeof(tbr0_str) - 1);
        assert(tbr0_build_record(tbr0_str, tbr0_str, &tbr0_recs[i]) == 0);
        assert(tbr0_insert(bt, tbr0_recs[i]) == 0);
        data++;
        bt_check(bt);
        //bt_dump(bt);
        //tbt_dump(bt);
    }
    
    // now build a large record with a large key
    recsz = bt_max_inline_record_size(bt);
    ksz = bt_max_inline_record_size(bt) - sizeof(btr_phys_t) - sizeof(uint64_t) - sizeof(bt_info_phys_t) - sizeof(uint64_t); // this is the max allowable keysize
    strsz = ksz - sizeof(tbr0_key_phys_t);
    assert(big_kstr = malloc(strsz));
    memset(big_kstr, 0, strsz);
    data = 33 + (n * 3 / 4);
    memset(big_kstr, data, strsz - 1);
    
    vsz = recsz - ksz - sizeof(btr_phys_t);
    strsz = vsz - sizeof(tbr0_val_phys_t);
    assert(vstr = malloc(strsz));
    memset(vstr, 0, strsz);
    memset(vstr, data, strsz - 1);
    
    // trigger the split:
    assert(tbr0_build_record(big_kstr, vstr, &tbr0_recs[n * 3 / 4]) == 0);
    assert(tbr0_insert(bt, tbr0_recs[n * 3 / 4]) == 0);
    bt_check(bt);
    //bt_dump(bt);
    
    // make sure we can find everything
    assert(tbr0_search_recs = malloc(sizeof(tbr0_phys_t *) * (n + 1)));
    memset(tbr0_search_recs, 0, sizeof(tbr0_phys_t *) * (n + 1));
    data = 33;
    for (int i = 0; i < n + 1; i++) {
        memset(tbr0_str, data, sizeof(tbr0_str) - 1);
        if (i == (n * 3 / 4))
            assert(tbr0_build_record(big_kstr, NULL, &tbr0_search_recs[i]) == 0);
        else
            assert(tbr0_build_record(tbr0_str, NULL, &tbr0_search_recs[i]) == 0);
        assert(tbr0_get(bt, tbr0_search_recs[i], &tbr0) == 0);
        tbr0p = tbr0_phys(tbr0);
        k = (uint8_t *)&tbr0_recs[i]->tbr0_key + sizeof(tbr0_key_phys_t);
        v = k + tbr0_recs[i]->tbr0_key.tbr0_kstrlen + sizeof(tbr0_val_phys_t);
        assert(!strcmp(k, (uint8_t *)tbr0->tbr0_key + sizeof(tbr0_key_phys_t)) &&
                !strcmp(v, (uint8_t *)tbr0->tbr0_val + sizeof(tbr0_val_phys_t)));
        tbr0_release(tbr0);
        data++;
    }
    
    // close the btree:
    assert(bt_close(bt) == 0);
    bt = NULL;
    
    assert(bt_check_disk(fname) == 0);
    //bt_dump_disk(fname);
    //tbt_dump_disk(fname);
    
    assert(bt_destroy(fname) == 0);
    
    free(big_kstr);
    free(vstr);
    for (int i = 0; i < n + 1; i++) {
        free(tbr0_search_recs[i]);
        free(tbr0_recs[i]);
    }
    free(tbr0_search_recs);
    free(tbr0_recs);
    free(fname);
}

static void test_node_splitting(void) {
    test_node_splitting_1();
    test_node_splitting_2();
    test_node_splitting_3();
    test_node_splitting_4();
    test_node_splitting_5();
    //test_node_splitting_random();
}

static void test_specific_cases(void) {
    test_specific_case1();
    test_node_splitting();
}

#define DEFAULT_NUM_ELEMENTS (1 << 10)

int main(int argc, char **argv) {
    unsigned int seed = 0;
    int ch, num_elements = DEFAULT_NUM_ELEMENTS;
    
    struct option longopts[] = {
        { "num",    required_argument,   NULL,   'n' },
        { "seed",   required_argument,   NULL,   's' },
        { NULL,                0,        NULL,    0 }
    };

    while ((ch = getopt_long(argc, argv, "", longopts, NULL)) != -1) {
        switch (ch) {
            case 'n':
                num_elements = (int)strtol(optarg, NULL, 10);
                break;
            case 's':
                seed = (unsigned int)strtol(optarg, NULL, 10);
                break;
            default:
                printf("usage: %s [--num <num-elements>] [--seed <seed>]\n", argv[0]);
                return -1;
        }
    }
    
    if (!seed) {
        seed = (unsigned int)time(NULL);
        srand(seed);
    } else {
        srand(seed);
    }
    
    printf("seed %u\n", seed);
    
    assert(mkdir(TEST_BTREE_DIR, S_IRWXU) == 0 || (errno == EEXIST));
    
    test_specific_cases();
    
    return 0;
}
