#ifndef _TBR1_H_
#define _TBR1_H_

#include "tbr.h"
#include "synch.h"

// 'test btree record 1s':

//
// on disk:
//

typedef struct
__attribute__((__packed__))
tbt_rec1_key_phys {
    tbr_key_phys_t tbr1_khdr;
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

int tbr1_phys_compare(tbr1_phys_t *rec1, tbr1_phys_t *rec2);
void tbr1_phys_dump(tbr1_phys_t *tbr1, bool key_only);


//
// in memory:
//

typedef struct tbt_rec1 {
    tbr_t tbr1_tbr;
    rw_lock_t *tbr1_rwlock;
} tbr1_t;

tbr1_phys_t *tbr1_phys(tbr1_t *tbr1);

int tbr1_insert(btree_t *bt, tbr1_phys_t *to_insert);
int tbr1_get(btree_t *bt, tbr1_phys_t *to_find, tbr1_t **record);
void tbr1_release(tbr1_t *tbr1);

int tbr1_build_record(uint64_t id, uint64_t data, tbr1_phys_t *record);

int tbr1_init(tbr1_phys_t *tbr1p, tbr1_t **tbr1);

#endif // _TBR1_H_
