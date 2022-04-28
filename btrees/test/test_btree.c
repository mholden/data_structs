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

#define TEST_BTREE_DIR "/var/tmp/test_btree"

static void tbt_dump_record(btr_phys_t *btr);

// tbrk_type's:
#define TBR_PHYS_KEY_TYPE_REC0 0
#define TBR_PHYS_KEY_TYPE_REC1 1

// all tbt_rec_phys key's start with this:
typedef struct
__attribute__((__packed__))
tbr_key_hdr_phys {
    uint32_t tbrk_type;
} tbr_key_hdr_phys_t;

// all tbt_rec_phys's start with this:
typedef struct
__attribute__((__packed__))
tbr_phys {
    btr_phys_t tbrp_btr;
    tbr_key_hdr_phys_t tbrp_khdr;
} tbr_phys_t;

static uint32_t tbr_phys_type(tbr_phys_t *tbrp) {
    return tbrp->tbrp_khdr.tbrk_type;
}

static const char *tbr_phys_type_to_string(uint32_t type) {
    switch (type) {
        case TBR_PHYS_KEY_TYPE_REC0:
            return "TBR_PHYS_KEY_TYPE_REC0";
        case TBR_PHYS_KEY_TYPE_REC1:
            return "TBR_PHYS_KEY_TYPE_REC1";
        default:
            return "ERROR: UNKOWN";
    }
}

static void tbr_phys_dump(tbr_phys_t *tbrp) {
    btr_phys_dump(&tbrp->tbrp_btr);
    printf("tbrk_type %s ", tbr_phys_type_to_string(tbr_phys_type(tbrp)));
}

typedef struct
__attribute__((__packed__))
tbt_rec0_key_phys {
    tbr_key_hdr_phys_t tbr0_khdr;
    uint16_t tbr0_kstrlen;
    //uint8_t tbr0_kstr[];
} tbr0_key_phys_t;

typedef struct
__attribute__((__packed__))
tbt_rec0_val_phys {
    uint16_t tbr0_vstrlen;
    //uint8_t tbr0_vstr[];
} tbr0_val_phys_t;

typedef struct
__attribute__((__packed__))
tbt_rec0_phys {
    btr_phys_t tbr0_btr;
    tbr0_key_phys_t tbr0_key;
    tbr0_val_phys_t tbr0_val;
} tbr0_phys_t;

static int tbr0_compare(tbr0_phys_t *rec1, tbr0_phys_t *rec2) {
    char *kstr1, *kstr2;
    
    kstr1 = (char *)((uint8_t *)&rec1->tbr0_key + sizeof(tbr0_key_phys_t));
    kstr2 = (char *)((uint8_t *)&rec2->tbr0_key + sizeof(tbr0_key_phys_t));
    
    return strcmp(kstr1, kstr2);
}

static void tbr0_dump(tbr0_phys_t *tbr0) {
    char *kstr, *vstr;
    tbr0_val_phys_t *tbr0_val;
    
    kstr = (char *)((uint8_t *)&tbr0->tbr0_key + sizeof(tbr0_key_phys_t));
    
    tbr0_val = (tbr0_val_phys_t *)((uint8_t *)&tbr0->tbr0_key + sizeof(tbr0_key_phys_t) + tbr0->tbr0_key.tbr0_kstrlen);
    vstr = (char *)((uint8_t *)tbr0_val + sizeof(tbr0_val_phys_t));
    
    tbr_phys_dump((tbr_phys_t *)tbr0);
    printf("tbr0_kstrlen %" PRIu16 " tbr0_kstr \"%s\" tbr0_vstrlen %" PRIu16 " tbr0_vstr \"%s\" ", tbr0->tbr0_key.tbr0_kstrlen, kstr, tbr0_val->tbr0_vstrlen, vstr);
}

typedef struct
__attribute__((__packed__))
tbt_rec1_key_phys {
    tbr_key_hdr_phys_t tbr1_khdr;
    uint64_t tbr1_id;
} tbr1_key_phys_t;

typedef struct
__attribute__((__packed__))
tbt_rec1_val_phys {
    uint64_t tbr1_data;
} tbr1_val_phys_t;

typedef struct
__attribute__((__packed__))
tbt_rec1_phys {
    btr_phys_t tbr1_btr;
    tbr1_key_phys_t tbr1_key;
    tbr1_val_phys_t tbr1_val;
} tbr1_phys_t;

static int tbr1_compare(tbr1_phys_t *rec1, tbr1_phys_t *rec2) {
    if (rec1->tbr1_key.tbr1_id > rec2->tbr1_key.tbr1_id)
        return 1;
    else if (rec1->tbr1_key.tbr1_id < rec2->tbr1_key.tbr1_id)
        return -1;
    else
        return 0;
}

static void tbr1_dump(tbr1_phys_t *tbr1) {
    tbr_phys_dump((tbr_phys_t *)tbr1);
    printf("tbr1_id %" PRIu64 " tbr1_data %" PRIu64 " ", tbr1->tbr1_key.tbr1_id, tbr1->tbr1_val.tbr1_data);
}

static int tbt_compare(btr_phys_t *btr1, btr_phys_t *btr2) {
    tbr_phys_t *tbrp1, *tbrp2;
    
    tbrp1 = (tbr_phys_t *)btr1;
    tbrp2 = (tbr_phys_t *)btr2;
    
    if (tbr_phys_type(tbrp1) > tbr_phys_type(tbrp2)) {
        return 1;
    } else if (tbr_phys_type(tbrp1) < tbr_phys_type(tbrp2)) {
        return -1;
    } else { // ==
        switch (tbr_phys_type(tbrp1)) {
            case TBR_PHYS_KEY_TYPE_REC0:
                return tbr0_compare((tbr0_phys_t *)tbrp1, (tbr0_phys_t *)tbrp2);
            case TBR_PHYS_KEY_TYPE_REC1:
                return tbr1_compare((tbr1_phys_t *)tbrp1, (tbr1_phys_t *)tbrp2);
            default:
                assert(0);
                return 0;
        }
    }
}

static void tbt_dump_record(btr_phys_t *btr) {
    tbr_phys_t *tbrp = (tbr_phys_t *)btr;
    switch (tbr_phys_type(tbrp)) {
        case TBR_PHYS_KEY_TYPE_REC0:
            tbr0_dump((tbr0_phys_t *)tbrp);
            break;
        case TBR_PHYS_KEY_TYPE_REC1:
            tbr1_dump((tbr1_phys_t *)tbrp);
            break;
        default:
            assert(0);
            break;
    }
}

static bt_ops_t tbt_bt_ops = {
    .bto_compare_fn = tbt_compare,
    .bto_dump_record_fn = tbt_dump_record,
    .bto_check_record_fn = NULL
};

// all tbt_rec's start with this
typedef struct tbt_rec {
    tbr_phys_t *tbr_phys;
} tbr_t;

static tbr_phys_t *tbr_phys(tbr_t *tbr) {
    return tbr->tbr_phys;
}

static uint32_t tbr_type(tbr_t *tbr) {
    return tbr_phys_type(tbr_phys(tbr));
}

typedef struct tbt_rec0 {
    tbr_t tbr0_tbr;
    rw_lock_t *tbr0_rwlock;
    tbr0_phys_t *tbr0_phys;
    tbr0_key_phys_t *tbr0_key;
    tbr0_val_phys_t *tbr0_val;
} tbr0_t;

typedef struct tbt_rec1 {
    tbr_t tbr1_tbr;
    rw_lock_t *tbr1_rwlock;
    tbr1_phys_t *tbr1_phys;
} tbr1_t;

static int tbr0_init(tbr0_phys_t *tbr0p, tbr0_t **tbr0) {
    tbr0_t *_tbr0 = NULL;
    int err;
    
    _tbr0 = malloc(sizeof(tbr0_t));
    if (!_tbr0) {
        err = ENOMEM;
        goto error_out;
    }
    
    memset(_tbr0, 0, sizeof(tbr0_t));
    _tbr0->tbr0_tbr.tbr_phys = (tbr_phys_t *)tbr0p;
    _tbr0->tbr0_rwlock = rwl_create();
    if (!_tbr0->tbr0_rwlock) {
        err = ENOMEM;
        goto error_out;
    }
    _tbr0->tbr0_phys = tbr0p;
    _tbr0->tbr0_key = &tbr0p->tbr0_key;
    _tbr0->tbr0_val = (tbr0_val_phys_t *)((uint8_t *)_tbr0->tbr0_key + sizeof(tbr0_key_phys_t) + _tbr0->tbr0_key->tbr0_kstrlen);
    
    *tbr0 = _tbr0;
    
    return 0;
    
error_out:
    if (_tbr0) {
        if (_tbr0->tbr0_rwlock)
            rwl_destroy(_tbr0->tbr0_rwlock);
        free(tbr0);
    }
    
    return err;
}

static void tbr0_release(tbr0_t *tbr0) {
    free(tbr0->tbr0_phys);
    rwl_destroy(tbr0->tbr0_rwlock);
    free(tbr0);
}

static int tbr1_init(tbr1_phys_t *tbr1p, tbr1_t **tbr1) {
    tbr1_t *_tbr1 = NULL;
    int err;
    
    _tbr1 = malloc(sizeof(tbr1_t));
    if (!_tbr1) {
        err = ENOMEM;
        goto error_out;
    }
    
    memset(_tbr1, 0, sizeof(tbr1_t));
    _tbr1->tbr1_tbr.tbr_phys = (tbr_phys_t *)tbr1p;
    _tbr1->tbr1_rwlock = rwl_create();
    if (!_tbr1->tbr1_rwlock) {
        err = ENOMEM;
        goto error_out;
    }
    _tbr1->tbr1_phys = tbr1p;
    
    *tbr1 = _tbr1;
    
    return 0;
    
error_out:
    if (_tbr1) {
        if (_tbr1->tbr1_rwlock)
            rwl_destroy(_tbr1->tbr1_rwlock);
        free(tbr1);
    }
    
    return err;
}

static void tbr1_release(tbr1_t *tbr1) {
    free(tbr1->tbr1_phys);
    rwl_destroy(tbr1->tbr1_rwlock);
    free(tbr1);
}

static int tbr_insert(btree_t *bt, tbr_phys_t *to_insert) {
    int err;
    
    err = bt_insert(bt, (btr_phys_t *)to_insert);
    if (err)
        goto error_out;
    
    return 0;
    
error_out:
    return err;
}

static void tbr_release(tbr_t *tbr) {
    switch (tbr_type(tbr)) {
        case TBR_PHYS_KEY_TYPE_REC0:
            tbr0_release((tbr0_t *)tbr);
            break;
        case TBR_PHYS_KEY_TYPE_REC1:
            tbr1_release((tbr1_t *)tbr);
            break;
        default:
            assert(0);
            break;
    }
    
    return;
}

static int tbr_get(btree_t *bt, tbr_phys_t *to_find, tbr_t **record) {
    tbr_t *tbr = NULL;
    tbr_phys_t *tbrp = NULL;
    int err;
    
    // TODO: implement a cache for synchronization
    
    err = bt_find(bt, (btr_phys_t *)to_find, (btr_phys_t **)&tbrp);
    if (err)
        goto error_out;
    
    assert(tbr_phys_type(to_find) == tbr_phys_type(tbrp));
    switch (tbr_phys_type(tbrp)) {
        case TBR_PHYS_KEY_TYPE_REC0:
            err = tbr0_init((tbr0_phys_t *)tbrp, (tbr0_t **)&tbr);
            break;
        case TBR_PHYS_KEY_TYPE_REC1:
            err = tbr1_init((tbr1_phys_t *)tbrp, (tbr1_t **)&tbr);
            break;
        default:
            err = EILSEQ;
            break;
    }
    if (err)
        goto error_out;
    
    *record = tbr;
    
    return 0;
    
error_out:
    if (tbr)
        tbr_release(tbr);
    else if (tbrp)
        free(tbrp);
    
    return err;
}

static int _tbt_dump_disk_record_cb(btr_phys_t *btr, void *ctx, bool *stop) {
    printf(" ");
    tbt_dump_record(btr);
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
    tbr0_val_phys_t *tbr0_val;
    size_t tbr0sz;
    tbr1_t *tbr1 = NULL;
    tbr1_phys_t tbr1p, search_tbr1p, *found_tbr1p;
    tbr1_key_phys_t tbr1_key;
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
    
    
    //
    // insert a tbr0 record:
    //
    
    tbr0sz = sizeof(tbr0_phys_t) + strlen(tbr0_kstr) + 1 + strlen(tbr0_vstr) + 1;
    assert(tbr0p = malloc(tbr0sz));
    memset(tbr0p, 0, tbr0sz);
    tbr0p->tbr0_btr.btrp_ksz = sizeof(tbr0_key_phys_t) + strlen(tbr0_kstr) + 1;
    tbr0p->tbr0_btr.btrp_vsz = sizeof(tbr0_val_phys_t) + strlen(tbr0_vstr) + 1;
    tbr0p->tbr0_key.tbr0_khdr.tbrk_type = TBR_PHYS_KEY_TYPE_REC0;
    tbr0p->tbr0_key.tbr0_kstrlen = strlen(tbr0_kstr) + 1;
    memcpy((uint8_t *)&tbr0p->tbr0_key + sizeof(tbr0_key_phys_t), tbr0_kstr, strlen(tbr0_kstr) + 1);
    tbr0_val = (tbr0_val_phys_t *)((uint8_t *)&tbr0p->tbr0_key + sizeof(tbr0_key_phys_t) + tbr0p->tbr0_key.tbr0_kstrlen);
    tbr0_val->tbr0_vstrlen = strlen(tbr0_vstr) + 1;
    memcpy((uint8_t *)tbr0_val + sizeof(tbr0_val_phys_t), tbr0_vstr, strlen(tbr0_vstr) + 1);
    
    assert(tbr_insert(bt, (tbr_phys_t *)tbr0p) == 0);
    //bt_dump(bt);
    
    //
    // check that  you can find it:
    //
    
    assert(search_tbr0p = malloc(tbr0sz));
    memset(search_tbr0p, 0, tbr0sz);
    search_tbr0p->tbr0_key.tbr0_khdr.tbrk_type = TBR_PHYS_KEY_TYPE_REC0;
    search_tbr0p->tbr0_key.tbr0_kstrlen = strlen(tbr0_kstr) + 1;
    memcpy((uint8_t *)&search_tbr0p->tbr0_key + sizeof(tbr0_key_phys_t), tbr0_kstr, strlen(tbr0_kstr) + 1);
    
    assert(tbr_get(bt, (tbr_phys_t *)search_tbr0p, (tbr_t **)&tbr0) == 0);
    
    
    //
    // and verify its integrity:
    //
    
    assert(strcmp((uint8_t *)tbr0->tbr0_key + sizeof(tbr0_key_phys_t), tbr0_kstr) == 0);
    assert(strcmp((uint8_t *)tbr0->tbr0_val + sizeof(tbr0_val_phys_t), tbr0_vstr) == 0);
    
    tbr_release((tbr_t *)tbr0);
    tbr0 = NULL;
    
    //bt_check(bt);
    //bt_dump(bt);
    
    
    //
    // insert a tbr1 record:
    //
    
    tbr1_id = (uint64_t)rand() << 32 | rand();
    tbr1_data = (uint64_t)rand() << 32 | rand();
    
    memset(&tbr1p, 0, sizeof(tbr1_phys_t));
    tbr1p.tbr1_btr.btrp_ksz = sizeof(tbr1_key_phys_t);
    tbr1p.tbr1_btr.btrp_vsz = sizeof(tbr1_val_phys_t);
    tbr1p.tbr1_key.tbr1_khdr.tbrk_type = TBR_PHYS_KEY_TYPE_REC1;
    tbr1p.tbr1_key.tbr1_id = tbr1_id;
    tbr1p.tbr1_val.tbr1_data = tbr1_data;
    
    assert(tbr_insert(bt, (tbr_phys_t *)&tbr1p) == 0);
    //bt_check(bt);
    //bt_dump(bt);
    
    
    //
    // check that  you can find it:
    //
    
    memset(&search_tbr1p, 0, sizeof(tbr1_phys_t));
    search_tbr1p.tbr1_key.tbr1_khdr.tbrk_type = TBR_PHYS_KEY_TYPE_REC1;
    search_tbr1p.tbr1_key.tbr1_id = tbr1_id;
    assert(tbr_get(bt, (tbr_phys_t *)&search_tbr1p, (tbr_t **)&tbr1) == 0);
    
    
    //
    // and verify its integrity:
    //
    
    found_tbr1p = tbr1->tbr1_phys;
    assert(found_tbr1p->tbr1_key.tbr1_id == tbr1_id);
    assert(found_tbr1p->tbr1_val.tbr1_data == tbr1_data);
    
    tbr_release((tbr_t *)tbr1);
    tbr1 = NULL;
    
    
    //
    // close the btree:
    //
    
    assert(bt_close(bt) == 0);
    bt = NULL;
    
    //assert(bt_check_disk(fname) == 0);
    //bt_dump_disk(fname);
    //tbt_dump_disk(fname);
    
    
    //
    // re-open and check that you can find your records:
    //
    
    assert(bt_open(fname, &tbt_bt_ops, &bt) == 0);
    
    //bt_check(bt);
    //bt_dump(bt);
    
    memset(search_tbr0p, 0, tbr0sz);
    search_tbr0p->tbr0_key.tbr0_khdr.tbrk_type = TBR_PHYS_KEY_TYPE_REC0;
    memcpy((uint8_t *)&search_tbr0p->tbr0_key + sizeof(tbr0_key_phys_t), tbr0_kstr, strlen(tbr0_kstr) + 1);
    
    assert(tbr_get(bt, (tbr_phys_t *)search_tbr0p, (tbr_t **)&tbr0) == 0);
    
    
    //
    // integrity check:
    //
    
    assert(strcmp((uint8_t *)tbr0->tbr0_key + sizeof(tbr0_key_phys_t), tbr0_kstr) == 0);
    assert(strcmp((uint8_t *)tbr0->tbr0_val + sizeof(tbr0_val_phys_t), tbr0_vstr) == 0);
    
    tbr_release((tbr_t *)tbr0);
    tbr0 = NULL;
    
    memset(&search_tbr1p, 0, sizeof(tbr1_phys_t));
    search_tbr1p.tbr1_key.tbr1_khdr.tbrk_type = TBR_PHYS_KEY_TYPE_REC1;
    search_tbr1p.tbr1_key.tbr1_id = tbr1_id;
    
    assert(tbr_get(bt, (tbr_phys_t *)&search_tbr1p, (tbr_t **)&tbr1) == 0);
    
    
    //
    // integrity check:
    //
    
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
