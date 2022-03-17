#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "avl-tree.h"

avl_tree_t *at_create(avl_tree_ops_t *ato) {
    avl_tree_t *at;
    
    at = malloc(sizeof(avl_tree_t));
    if (!at) {
        printf("at_create: couldn't allocate at\n");
        goto error_out;
    }
    memset(at, 0, sizeof(*at));
    at->at_ops = ato;
    
    return at;
    
error_out:
    return NULL;
}

static void _at_destroy(avl_tree_t *at, avl_tree_node_t *atn) {
    if (!atn) return;
    _at_destroy(at, atn->atn_lchild);
    _at_destroy(at, atn->atn_rchild);
    at->at_ops->ato_destroy_data_fn(atn->atn_data);
    free(atn);
}

void at_destroy(avl_tree_t *at) {
    _at_destroy(at, at->at_root);
    free(at);
    return;
}

static void _at_check(avl_tree_t *at, avl_tree_node_t *atn, int *height) {
    avl_tree_node_t *rchild, *lchild;
    int rheight = 0, lheight = 0, sub_height, balance;
    
    if (!atn) return;
    
    rchild = atn->atn_rchild;
    lchild = atn->atn_lchild;
    if (rchild) {
        assert(at->at_ops->ato_compare_fn(rchild->atn_data, atn->atn_data) >= 0);
        _at_check(at, rchild, &rheight);
    }
    if (lchild) {
        assert(at->at_ops->ato_compare_fn(lchild->atn_data, atn->atn_data) < 0);
        _at_check(at, lchild, &lheight);
    }
    
    balance = rheight - lheight;
    assert((atn->atn_balance == balance) && (abs(balance) <= 1));
    sub_height = (lheight > rheight) ? lheight : rheight;
    if (height)
        *height = sub_height + 1;
    
    return;
}

void at_check(avl_tree_t *at) {
    _at_check(at, at->at_root, NULL);
}

static bool atn_is_right_heavy(avl_tree_node_t *atn) {
    if (atn->atn_balance > 0)
        return true;
    else
        return false;
}

static bool atn_is_left_heavy(avl_tree_node_t *atn) {
    if (atn->atn_balance < 0)
        return true;
    else
        return false;
}

static bool atn_is_perfectly_balanced(avl_tree_node_t *atn) {
    if (atn->atn_balance == 0)
        return true;
    else
        return false;
}

static bool atn_unbalanced(avl_tree_node_t *atn) {
    if (abs(atn->atn_balance) > 1)
        return true;
    else
        return false;
}

static int at_rotate_right(avl_tree_t *at, avl_tree_node_t *atn, avl_tree_node_t *parent) {
    avl_tree_node_t *child, *grandchild;
    int err = -1;
    
    assert(atn_is_left_heavy(atn));
    
    child = atn->atn_lchild;
    grandchild = child->atn_rchild;
    
    if (!parent) { // parent == root
        assert(atn == at->at_root);
        at->at_root = child;
    } else {
        if (at->at_ops->ato_compare_fn(atn->atn_data, parent->atn_data) >= 0) // right side of our parent
            parent->atn_rchild = child;
        else // left side of our parent
            parent->atn_lchild = child;
    }
    child->atn_rchild = atn;
    atn->atn_lchild = grandchild;
    
    //
    // right rotation results in either a right heavy tree/subtree or a balanced
    // tree/subtree. child is now the root of the rotated subtree. atn is now
    // its right child.
    //
    
    // deal with adjusting atn's balance first
    if (!atn->atn_rchild && !atn->atn_lchild) {
        atn->atn_balance = 0;
    } else if (atn->atn_rchild && atn->atn_lchild) {
        assert(atn_is_perfectly_balanced(atn->atn_rchild) && atn_is_perfectly_balanced(atn->atn_lchild));
        atn->atn_balance = 0;
    } else { // one child -> atn is right heavy
        assert(atn->atn_rchild && !atn->atn_lchild && atn_is_perfectly_balanced(atn->atn_rchild));
        atn->atn_balance = 1;
    }
    
    // now deal with child
    if ((child->atn_rchild && child->atn_lchild)) {
        if (atn->atn_balance == 1) {
            assert(atn_is_perfectly_balanced(child->atn_lchild) && atn_is_left_heavy(child));
            child->atn_balance += 2;
        } else {
            //assert(atn_is_perfectly_balanced(child->atn_rchild) && atn_is_perfectly_balanced(child->atn_lchild));
            child->atn_balance = 0;
        }
    } else {
        assert(child->atn_rchild && !child->atn_lchild && atn_is_perfectly_balanced(atn) && parent && atn_unbalanced(parent));
        if (atn->atn_lchild && atn->atn_rchild) {
            // in this case, child becomes unbalanced
            child->atn_balance = 2;
        } else {
            assert(!atn->atn_lchild && !atn->atn_rchild);
            child->atn_balance = 1;
        }
    }
    
    return 0;
    
error_out:
    return err;
}

static int at_rotate_left(avl_tree_t *at, avl_tree_node_t *atn, avl_tree_node_t *parent) {
    avl_tree_node_t *child, *grandchild;
    int err = -1;
    
    assert(atn_is_right_heavy(atn));
    
    child = atn->atn_rchild;
    grandchild = child->atn_lchild;
    
    if (!parent) { // parent == root
        assert(atn == at->at_root);
        at->at_root = child;
    } else {
        if (at->at_ops->ato_compare_fn(atn->atn_data, parent->atn_data) >= 0) // right side of our parent
            parent->atn_rchild = child;
        else // left side of our parent
            parent->atn_lchild = child;
    }
    child->atn_lchild = atn;
    atn->atn_rchild = grandchild;
    
    //
    // left rotation results in either a left heavy tree/subtree or a balanced
    // tree/subtree. child is now the root of the rotated subtree. atn is now
    // its left child.
    //
    
    // deal with adjusting atn's balance first
    if (!atn->atn_rchild && !atn->atn_lchild) {
        atn->atn_balance = 0;
    } else if (atn->atn_rchild && atn->atn_lchild) {
        assert(atn_is_perfectly_balanced(atn->atn_rchild) && atn_is_perfectly_balanced(atn->atn_lchild));
        atn->atn_balance = 0;
    } else { // one child -> atn is left heavy
        assert(atn->atn_lchild && !atn->atn_rchild && atn_is_perfectly_balanced(atn->atn_lchild));
        atn->atn_balance = -1;
    }
    
    // now deal with child
    if ((child->atn_rchild && child->atn_lchild)) {
        if (atn->atn_balance == -1) {
            assert(atn_is_perfectly_balanced(child->atn_rchild) && atn_is_right_heavy(child));
            child->atn_balance -= 2;
        } else {
            //assert(atn_is_perfectly_balanced(child->atn_rchild) && atn_is_perfectly_balanced(child->atn_lchild));
            child->atn_balance = 0;
        }
    } else {
        assert(child->atn_lchild && !child->atn_rchild && atn_is_perfectly_balanced(atn) && parent && atn_unbalanced(parent));
        if (atn->atn_lchild && atn->atn_rchild) {
            // in this case, child becomes unbalanced
            child->atn_balance = -2;
        } else {
            assert(!atn->atn_lchild && !atn->atn_rchild);
            child->atn_balance = -1;
        }
    }
    
    return 0;
    
error_out:
    return err;
}

static int at_balance(avl_tree_t *at, avl_tree_node_t *atn, avl_tree_node_t *parent) {
    avl_tree_node_t *child;
    int err = -1;
    
    assert(atn_unbalanced(atn));
    
    if (atn_is_right_heavy(atn)) {
        child = atn->atn_rchild;
        if (atn_is_left_heavy(child)) {
            // need the extra right rotation on child first
            err = at_rotate_right(at, child, atn);
            if (err)
                goto error_out;
        }
        err = at_rotate_left(at, atn, parent);
        if (err)
            goto error_out;
    } else { // atn_is_left_heavy
        assert(atn_is_left_heavy(atn));
        child = atn->atn_lchild;
        if (atn_is_right_heavy(child)) {
            // need the extra left rotation on child first
            err = at_rotate_left(at, child, atn);
            if (err)
                goto error_out;
        }
        err = at_rotate_right(at, atn, parent);
        if (err)
            goto error_out;
    }
    
    return 0;
    
error_out:
    return err;
}

static int _at_insert(avl_tree_t *at, avl_tree_node_t *atn, avl_tree_node_t *parent, avl_tree_node_t *to_insert, bool *height_change) {
    bool rheight_change = false, lheight_change = false;
    int err = -1;
    
    if (at->at_ops->ato_compare_fn(to_insert->atn_data, atn->atn_data) == 0) {
        err = EEXIST;
        goto error_out;
    }
    
    if (at->at_ops->ato_compare_fn(to_insert->atn_data, atn->atn_data) > 0) { // insert on right
        if (!atn->atn_rchild) {
            atn->atn_rchild = to_insert;
            rheight_change = true;
        } else {
            err = _at_insert(at, atn->atn_rchild, atn, to_insert, &rheight_change);
            if (err)
                goto error_out;
        }
    } else { // insert on left
        if (!atn->atn_lchild) {
            atn->atn_lchild = to_insert;
            lheight_change = true;
        } else {
            err = _at_insert(at, atn->atn_lchild, atn, to_insert, &lheight_change);
            if (err)
                goto error_out;
        }
    }
    
    if (rheight_change) {
        if (!atn_is_left_heavy(atn)) {
            //
            // if balanced or right heavy, then a height change
            // of our right subtree changes our height too
            //
            if (height_change)
                *height_change = true;
        }
        atn->atn_balance++;
    } else if (lheight_change) {
        if (!atn_is_right_heavy(atn)) { // same thing on the left side
            if (height_change)
                *height_change = true;
        }
        atn->atn_balance--;
    }
    
    if (atn_unbalanced(atn)) {
        assert(rheight_change || lheight_change);
        at_balance(at, atn, parent);
        //
        // balancing reduces the height of the atn subtree by 1.
        // height changes can only occur in increments of 1 (we're
        // only inserting 1 node here). so, a height change on one
        // of our subtrees (rheight_change || lheight_change) followed
        // by a rebalance (via at_balance) results in a net height
        // change of 0
        //
        if (height_change)
            *height_change = false;
    }
    
out:
    return 0;
    
error_out:
    return err;
}

int at_insert(avl_tree_t *at, void *data) {
    avl_tree_node_t *atn = NULL;
    int err = -1;
    
    atn = malloc(sizeof(avl_tree_node_t));
    if (!atn) {
        printf("at_insert: malloc failed to allocate memory for atn\n");
        goto error_out;
    }
    
    memset(atn, 0, sizeof(*atn));
    atn->atn_data = data;
    
    if (!at->at_root) {
        at->at_root = atn;
    } else {
        err = _at_insert(at, at->at_root, NULL, atn, NULL);
        if (err)
            goto error_out;
    }
    
    at->at_nnodes++;

    //at_check(at);

    return 0;
    
error_out:
    if (atn)
        free(atn);
    
    return err;
}

// TODO: implement
int at_find(avl_tree_t *at, void *data) {
    
}

// TODO: implement
int at_remove(avl_tree_t *at, void *data) {
    
#if 0
    at_check(at);
#endif
    
    return 0;
}

static void _at_dump(avl_tree_t *at, avl_tree_node_t *atn, int *height) {
    avl_tree_node_t *rchild, *lchild;
    int rheight = 0, lheight = 0, sub_height;
    
    if (!atn) return;
    
    rchild = atn->atn_rchild;
    lchild = atn->atn_lchild;
    if (rchild)
        _at_dump(at, rchild, &rheight);
    if (lchild)
        _at_dump(at, lchild, &lheight);
    
    sub_height = (lheight > rheight) ? lheight : rheight;
    *height = sub_height + 1;
    
    printf("** node @ %p (height %d) **\n", atn, *height);
    printf("atn_balance: %d\n", atn->atn_balance);
    printf("atn_data:\n");
    at->at_ops->ato_dump_data_fn(atn->atn_data);
    printf("atn_lchild %p\n", lchild);
    printf("atn_rchild %p\n", rchild);
    
    return;
}

void at_dump(avl_tree_t *at) {
    int height;
    
    printf("***** at @ %p *****\n", at);
    printf("at_nnodes: %d\n", at->at_nnodes);
    _at_dump(at, at->at_root, &height);
    printf("\n");
}
