#ifndef _TBR0_H_
#define _TBR0_H_

#include "tbr.h"
#include "synch.h"

// 'test btree record 0s':

//
// on disk:
//

typedef struct
__attribute__((__packed__))
tbt_rec0_key_phys {
    tbr_key_phys_t tbr0_khdr;
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
    tbr0_val_phys_t tbr0_val; // tbr0_key is variable length, so don't use this
} tbr0_phys_t;

int tbr0_phys_compare(tbr0_phys_t *rec1, tbr0_phys_t *rec2);
void tbr0_phys_dump(tbr0_phys_t *tbr0, bool key_only);


//
// in memory:
//

typedef struct tbt_rec0 {
    tbr_t tbr0_tbr;
    rw_lock_t *tbr0_rwlock;
    tbr0_key_phys_t *tbr0_key;
    tbr0_val_phys_t *tbr0_val;
} tbr0_t;

tbr0_phys_t *tbr0_phys(tbr0_t *tbr0);

int tbr0_insert(btree_t *bt, tbr0_phys_t *to_insert);
int tbr0_get(btree_t *bt, tbr0_phys_t *to_find, tbr0_t **record);
int tbr0_remove(btree_t *bt, tbr0_phys_t *to_remove);
void tbr0_release(tbr0_t *tbr0);

int tbr0_build_record(const char *kstr, const char *vstr, tbr0_phys_t **record);

int tbr0_init(tbr0_phys_t *tbr0p, tbr0_t **tbr0);

#endif // _TBR0_H_
