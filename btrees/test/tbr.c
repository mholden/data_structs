#include <stdio.h>
#include <inttypes.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include "btree.h"
#include "tbr.h"
#include "tbr0.h"
#include "tbr1.h"

uint32_t tbr_phys_type(tbr_phys_t *tbrp) {
    return tbrp->tbrp_khdr.tbrk_type;
}

const char *tbr_phys_type_to_string(uint32_t type) {
    switch (type) {
        case TBR_PHYS_KEY_TYPE_REC0:
            return "TBR_PHYS_KEY_TYPE_REC0";
        case TBR_PHYS_KEY_TYPE_REC1:
            return "TBR_PHYS_KEY_TYPE_REC1";
        default:
            return "ERROR: UNKOWN";
    }
}

void tbr_phys_dump(tbr_phys_t *tbrp) {
    btr_phys_dump(&tbrp->tbrp_btr);
    printf("tbrk_type %s ", tbr_phys_type_to_string(tbr_phys_type(tbrp)));
    switch (tbr_phys_type(tbrp)) {
        case TBR_PHYS_KEY_TYPE_REC0:
            tbr0_phys_dump((tbr0_phys_t *)tbrp);
            break;
        case TBR_PHYS_KEY_TYPE_REC1:
            tbr1_phys_dump((tbr1_phys_t *)tbrp);
            break;
        default:
            assert(0);
            break;
    }
}

tbr_phys_t *tbr_phys(tbr_t *tbr) {
    return tbr->tbr_phys;
}

int tbr_phys_compare(tbr_phys_t *tbrp1, tbr_phys_t *tbrp2) {
    if (tbr_phys_type(tbrp1) > tbr_phys_type(tbrp2)) {
        return 1;
    } else if (tbr_phys_type(tbrp1) < tbr_phys_type(tbrp2)) {
        return -1;
    } else { // ==
        switch (tbr_phys_type(tbrp1)) {
            case TBR_PHYS_KEY_TYPE_REC0:
                return tbr0_phys_compare((tbr0_phys_t *)tbrp1, (tbr0_phys_t *)tbrp2);
            case TBR_PHYS_KEY_TYPE_REC1:
                return tbr1_phys_compare((tbr1_phys_t *)tbrp1, (tbr1_phys_t *)tbrp2);
            default:
                assert(0);
                return 0;
        }
    }
}

uint32_t tbr_type(tbr_t *tbr) {
    return tbr_phys_type(tbr_phys(tbr));
}

int tbr_insert(btree_t *bt, tbr_phys_t *to_insert) {
    int err;
    
    err = bt_insert(bt, (btr_phys_t *)to_insert);
    if (err)
        goto error_out;
    
    return 0;
    
error_out:
    return err;
}

int tbr_get(btree_t *bt, tbr_phys_t *to_find, tbr_t **record) {
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

void tbr_release(tbr_t *tbr) {
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
