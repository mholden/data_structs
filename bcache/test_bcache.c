#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>

#include "bcache.h"

#define TEST_BCACHE_DIR "/var/tmp/test_bcache"
#define TEST_BCACHE_BLKSZ 1024

#if 0
#define TEST_BCACHE_FILESZ 8 * TEST_BCACHE_BLKSZ
#define TEST_BCACHE_MAXSZ 4 * TEST_BCACHE_BLKSZ
#else
#define TEST_BCACHE_FILESZ 1024 * TEST_BCACHE_BLKSZ
#define TEST_BCACHE_MAXSZ 128 * TEST_BCACHE_BLKSZ
#endif

#define TEST_BCACHE_NFBLOCKS (TEST_BCACHE_FILESZ / TEST_BCACHE_BLKSZ)
#define TEST_BCACHE_NCBLOCKS (TEST_BCACHE_MAXSZ / TEST_BCACHE_BLKSZ)

typedef struct tbco_phys { // btree node phys, for example
    uint64_t bcp_data;
    //uint8_t tbop_data[];
} tbco_phys_t;

typedef struct test_bcache_object { // btree node, for example
    block_t *bco_block;
    tbco_phys_t *bco_phys;
} tbco_t;

static int tbco_init(void **bco, block_t *b) {
    tbco_t **tbco, *_tbco;
    tbco_phys_t *tbcop = (tbco_phys_t *)b->bl_data;
    int err;
    
    tbco = (tbco_t **)bco;
    
    _tbco = malloc(sizeof(tbco_t));
    if (!_tbco) {
        err = ENOMEM;
        goto error_out;
    }
    
    memset(_tbco, 0, sizeof(tbco_t));
    _tbco->bco_block = b;
    _tbco->bco_phys = tbcop;
    
    *tbco = _tbco;
    
    return 0;
    
error_out:
    return err;
}

static void tbco_destroy(void *bco) {
    tbco_t *tbco = (tbco_t *)bco;
    free(tbco);
}

static bco_ops_t tbco_ops = {
    .bco_init = tbco_init,
    .bco_destroy = tbco_destroy,
    .bco_dump = NULL,
    .bco_check = NULL
};

static block_t *tbco_block(tbco_t *tbco) {
    return tbco->bco_block;
}

static void create_and_init_backing_file(const char *fname) {
    int fd;
    uint64_t data;
    
    assert(unlink(fname) == 0 || (errno == ENOENT));
    
    fd = open(fname, O_CREAT | O_RDWR);
    assert(fd >= 0);
    
    assert(posix_fallocate(fd, 0, TEST_BCACHE_FILESZ) == 0);
    
    for (int i = 0; i < TEST_BCACHE_NFBLOCKS; i++) {
        data = (uint64_t)i;
        assert(pwrite(fd, &data, sizeof(uint64_t), TEST_BCACHE_BLKSZ * i) == sizeof(uint64_t));
    }
    
    assert(close(fd) == 0);
}

//
// get, modify and release all blocks in the file
// will cause blocks to get evicted and flushed
//
static void test_bcache2(void) {
    bcache_t *bc;
    tbco_t *tbco = NULL;
    tbco_phys_t *tbcop;
    char *tname = "test_bcache2", *fname;
    
    printf("%s\n", tname);
    
    assert(fname = malloc(strlen(TEST_BCACHE_DIR) + strlen(tname) + 2));
    sprintf(fname, "%s/%s", TEST_BCACHE_DIR, tname);
    
    create_and_init_backing_file(fname);
    
    assert(bc = bc_create(fname, TEST_BCACHE_BLKSZ, TEST_BCACHE_MAXSZ, &tbco_ops));
    
    for (int i = 0; i < TEST_BCACHE_NFBLOCKS; i++) {
        assert(bc_get(bc, i, (void **)&tbco) == 0);
        tbcop = tbco->bco_phys;
        assert(tbcop->bcp_data == (uint64_t)i);
        tbcop->bcp_data++;
        bc_dirty(bc, tbco_block(tbco));
        bc_check(bc);
        //bc_dump(bc);
        bc_release(bc, tbco_block(tbco));
        bc_check(bc);
        //bc_dump(bc);
    }
    
    for (int i = 0; i < TEST_BCACHE_NFBLOCKS; i++) {
        assert(bc_get(bc, i, (void **)&tbco) == 0);
        tbcop = tbco->bco_phys;
        assert(tbcop->bcp_data == (uint64_t)(i + 1));
        bc_release(bc, tbco_block(tbco));
    }
    
    bc_check(bc);
    
    bc_destroy(bc);
    free(fname);
}

//
// simply get and release all blocks in the file
// will cause blocks to get evicted, but none will be dirty
//
static void test_bcache1(void) {
    bcache_t *bc;
    tbco_t *tbco = NULL;
    tbco_phys_t *tbcop;
    char *tname = "test_bcache1", *fname;
    
    printf("%s\n", tname);
    
    assert(fname = malloc(strlen(TEST_BCACHE_DIR) + strlen(tname) + 2));
    sprintf(fname, "%s/%s", TEST_BCACHE_DIR, tname);
    
    create_and_init_backing_file(fname);
    
    assert(bc = bc_create(fname, TEST_BCACHE_BLKSZ, TEST_BCACHE_MAXSZ, &tbco_ops));
    
    for (int i = 0; i < TEST_BCACHE_NFBLOCKS; i++) {
        assert(bc_get(bc, i, (void **)&tbco) == 0);
        tbcop = tbco->bco_phys;
        assert(tbcop->bcp_data == (uint64_t)i);
        bc_check(bc);
        //bc_dump(bc);
        bc_release(bc, tbco_block(tbco));
        bc_check(bc);
        //bc_dump(bc);
    }
    
    bc_destroy(bc);
    free(fname);
}

#define DEFAULT_NUM_ELEMENTS (1 << 10)

int main(int argc, char **argv) {
    unsigned int seed = 0;
    int ch, num_elements = DEFAULT_NUM_ELEMENTS, err;
    
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
    } else
        srand(seed);
    
    printf("seed %u\n", seed);
    
    assert(mkdir(TEST_BCACHE_DIR, S_IRWXU) == 0 || (errno == EEXIST));
    
    test_bcache1();
    test_bcache2();
    
    return 0;
}
