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

static bt_ops_t tbt_bt_ops = {
    .bto_compare_fn = tbt_compare,
    .bto_dump_record_fn = NULL,
    .bto_check_record_fn = NULL
};

static int _tbt_dump_disk_record_cb(btr_phys_t *btr, void *ctx, bool *stop) {
    tbr_phys_t *tbrp = (tbr_phys_t *)btr;
    printf(" ");
    tbr_phys_dump(tbrp);
    printf("\n");
    return 0;
}

static void tbt_dump_disk(const char *path) {
    printf("tbt records @ %s:\n", path);
    bt_iterate_disk(path, NULL, NULL, _tbt_dump_disk_record_cb, NULL);
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
    
    //assert(bt_check_disk(fname) == 0);
    //bt_dump_disk(fname);
    
    assert(bt_open(fname, &tbt_bt_ops, &bt) == 0);
    //bt_check(bt);
    //bt_dump(bt);
    
    // insert a tbr0 record:
    assert(tbr0_build_record(tbr0_kstr, tbr0_vstr, &tbr0p) == 0);
    assert(tbr0_insert(bt, tbr0p) == 0);
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
    //bt_check(bt);
    //bt_dump(bt);
    
    // check that  you can find it:
    assert(tbr1_build_record(tbr1_id, 0, &search_tbr1p) == 0);
    assert(tbr1_get(bt, &search_tbr1p, &tbr1) == 0);
    
    // and verify its integrity:
    found_tbr1p = tbr1->tbr1_phys;
    assert(found_tbr1p->tbr1_key.tbr1_id == tbr1_id);
    assert(found_tbr1p->tbr1_val.tbr1_data == tbr1_data);
    
    tbr1_release(tbr1);
    tbr1 = NULL;
    
    // close the btree:
    assert(bt_close(bt) == 0);
    bt = NULL;
    
    //assert(bt_check_disk(fname) == 0);
    //bt_dump_disk(fname);
    //tbt_dump_disk(fname);
    
    // re-open and check that you can find your records:
    assert(bt_open(fname, &tbt_bt_ops, &bt) == 0);
    
    //bt_check(bt);
    //bt_dump(bt);
    
    assert(tbr0_get(bt, search_tbr0p, &tbr0) == 0);
    assert(strcmp((uint8_t *)tbr0->tbr0_key + sizeof(tbr0_key_phys_t), tbr0_kstr) == 0);
    assert(strcmp((uint8_t *)tbr0->tbr0_val + sizeof(tbr0_val_phys_t), tbr0_vstr) == 0);
    
    tbr0_release(tbr0);
    tbr0 = NULL;
    
    
    assert(tbr1_get(bt, &search_tbr1p, &tbr1) == 0);
    found_tbr1p = tbr1->tbr1_phys;
    assert(found_tbr1p->tbr1_key.tbr1_id == tbr1_id);
    assert(found_tbr1p->tbr1_val.tbr1_data == tbr1_data);
    
    tbr_release((tbr_t *)tbr1);
    tbr1 = NULL;
    
    assert(bt_close(bt) == 0);
    bt = NULL;
    
    assert(bt_destroy(fname) == 0);
    
    free(tbr0p);
    free(search_tbr0p);
    free(fname);
}

static void test_specific_cases(void) {
    test_specific_case1();
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
