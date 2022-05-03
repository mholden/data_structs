#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "tbr1.h"

int tbr1_phys_compare(tbr1_phys_t *rec1, tbr1_phys_t *rec2) {
    if (rec1->tbr1_key.tbr1_id > rec2->tbr1_key.tbr1_id)
        return 1;
    else if (rec1->tbr1_key.tbr1_id < rec2->tbr1_key.tbr1_id)
        return -1;
    else
        return 0;
}

void tbr1_phys_dump(tbr1_phys_t *tbr1, bool key_only) {
    printf("tbr1_id %" PRIu64 " ", tbr1->tbr1_key.tbr1_id);
    if (!key_only)
        printf("tbr1_data %" PRIu64 " ", tbr1->tbr1_val.tbr1_data);
}

int tbr1_init(tbr1_phys_t *tbr1p, tbr1_t **tbr1) {
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

int tbr1_build_record(uint64_t id, uint64_t data, tbr1_phys_t *record){
    tbr1_phys_t tbr1p;
    
    memset(&tbr1p, 0, sizeof(tbr1_phys_t));
    tbr1p.tbr1_btr.btrp_ksz = sizeof(tbr1_key_phys_t);
    tbr1p.tbr1_btr.btrp_vsz = sizeof(tbr1_val_phys_t);
    tbr1p.tbr1_key.tbr1_khdr.tbrk_type = TBR_PHYS_KEY_TYPE_REC1;
    tbr1p.tbr1_key.tbr1_id = id;
    tbr1p.tbr1_val.tbr1_data = data;
    
    *record = tbr1p;
    
    return 0;
}

tbr1_phys_t *tbr1_phys(tbr1_t *tbr1) {
    return (tbr1_phys_t *)tbr1->tbr1_tbr.tbr_phys;
}

int tbr1_insert(btree_t *bt, tbr1_phys_t *to_insert) {
    return tbr_insert(bt, (tbr_phys_t *)to_insert);
}

int tbr1_get(btree_t *bt, tbr1_phys_t *to_find, tbr1_t **record) {
    return tbr_get(bt, (tbr_phys_t *)to_find, (tbr_t **)record);
}

void tbr1_release(tbr1_t *tbr1) {
    free(tbr1_phys(tbr1));
    rwl_destroy(tbr1->tbr1_rwlock);
    free(tbr1);
}
