#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "tbr0.h"

int tbr0_phys_compare(tbr0_phys_t *rec1, tbr0_phys_t *rec2) {
    char *kstr1, *kstr2;
    
    kstr1 = (char *)((uint8_t *)&rec1->tbr0_key + sizeof(tbr0_key_phys_t));
    kstr2 = (char *)((uint8_t *)&rec2->tbr0_key + sizeof(tbr0_key_phys_t));
    
    return strcmp(kstr1, kstr2);
}

void tbr0_phys_dump(tbr0_phys_t *tbr0, bool key_only) {
    char *kstr, *vstr;
    tbr0_val_phys_t *tbr0_val;
    
    kstr = (char *)((uint8_t *)&tbr0->tbr0_key + sizeof(tbr0_key_phys_t));
    printf("tbr0_kstrlen %" PRIu16 " tbr0_kstr \"%s\" ", tbr0->tbr0_key.tbr0_kstrlen, kstr);
    
    if (!key_only) {
        tbr0_val = (tbr0_val_phys_t *)((uint8_t *)&tbr0->tbr0_key + sizeof(tbr0_key_phys_t) + tbr0->tbr0_key.tbr0_kstrlen);
        vstr = (char *)((uint8_t *)tbr0_val + sizeof(tbr0_val_phys_t));
        printf("tbr0_vstrlen %" PRIu16 " tbr0_vstr \"%s\" ", tbr0_val->tbr0_vstrlen, vstr);
    }
}

int tbr0_build_record(const char *kstr, const char *vstr, tbr0_phys_t **record){
    tbr0_phys_t *tbr0p;
    size_t klen, vlen = 0, tbr0sz;
    tbr0_val_phys_t *tbr0_val;
    int err;
    
    klen = strlen(kstr) + 1;
    if (vstr)
        vlen = strlen(vstr) + 1;
    
    tbr0sz = sizeof(tbr0_phys_t) + klen + vlen;
    
    tbr0p = malloc(tbr0sz);
    if (!tbr0p) {
        err = ENOMEM;
        goto error_out;
    }
    
    memset(tbr0p, 0, tbr0sz);
    tbr0p->tbr0_btr.btrp_ksz = sizeof(tbr0_key_phys_t) + klen;
    if (vstr)
        tbr0p->tbr0_btr.btrp_vsz = sizeof(tbr0_val_phys_t) + vlen;
    tbr0p->tbr0_key.tbr0_khdr.tbrk_type = TBR_PHYS_KEY_TYPE_REC0;
    tbr0p->tbr0_key.tbr0_kstrlen = klen;
    memcpy((uint8_t *)&tbr0p->tbr0_key + sizeof(tbr0_key_phys_t), kstr, klen);
    if (vstr) {
        tbr0_val = (tbr0_val_phys_t *)((uint8_t *)&tbr0p->tbr0_key + sizeof(tbr0_key_phys_t) + tbr0p->tbr0_key.tbr0_kstrlen);
        tbr0_val->tbr0_vstrlen = vlen;
        memcpy((uint8_t *)tbr0_val + sizeof(tbr0_val_phys_t), vstr, vlen);
    }
    
    *record = tbr0p;
    
    return 0;
    
error_out:
    return err;
}

tbr0_phys_t *tbr0_phys(tbr0_t *tbr0) {
    return (tbr0_phys_t *)tbr0->tbr0_tbr.tbr_phys;
}

int tbr0_init(tbr0_phys_t *tbr0p, tbr0_t **tbr0) {
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

int tbr0_insert(btree_t *bt, tbr0_phys_t *to_insert) {
    return tbr_insert(bt, (tbr_phys_t *)to_insert);
}

int tbr0_get(btree_t *bt, tbr0_phys_t *to_find, tbr0_t **record) {
    return tbr_get(bt, (tbr_phys_t *)to_find, (tbr_t **)record);
}

void tbr0_release(tbr0_t *tbr0) {
    free(tbr0_phys(tbr0));
    rwl_destroy(tbr0->tbr0_rwlock);
    free(tbr0);
}
