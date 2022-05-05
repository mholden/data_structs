#ifndef _TBR_H_
#define _TBR_H_

#include <inttypes.h>

#include "btree.h"

// 'test btree records':

//
// on disk:
//

// tbrk_type's:
#define TBR_PHYS_KEY_TYPE_REC0 0
#define TBR_PHYS_KEY_TYPE_REC1 1

// all tbt_rec_phys key's start with this:
typedef struct
__attribute__((__packed__))
tbr_key_phys {
    uint32_t tbrk_type;
} tbr_key_phys_t;

// all tbt_rec_phys's start with this:
typedef struct
__attribute__((__packed__))
tbr_phys {
    btr_phys_t tbrp_btr;
    tbr_key_phys_t tbrp_khdr;
} tbr_phys_t;

uint32_t tbr_phys_type(tbr_phys_t *tbrp);
const char *tbr_phys_type_to_string(uint32_t type);

int tbr_phys_compare(tbr_phys_t *tbrp1, tbr_phys_t *tbrp2);
void tbr_phys_dump(tbr_phys_t *tbrp, bool key_only);


//
// in memory:
//

// all tbt_rec's start with this
typedef struct tbt_rec {
    tbr_phys_t *tbr_phys;
} tbr_t;

tbr_phys_t *tbr_phys(tbr_t *tbr);
uint32_t tbr_type(tbr_t *tbr);

int tbr_insert(btree_t *bt, tbr_phys_t *to_insert);
int tbr_get(btree_t *bt, tbr_phys_t *to_find, tbr_t **record);
int tbr_remove(btree_t *bt, tbr_phys_t *to_remove);
void tbr_release(tbr_t *tbr);

#endif // _TBR_H_
