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
        assert(at->at_ops->ato_compare_fn(rchild->atn_data, atn->atn_data) > 0);
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

static bool atn_has_no_children(avl_tree_node_t *atn) {
    if (!atn->atn_lchild && !atn->atn_rchild)
        return true;
    else
        return false;
}

static int at_rotate_right(avl_tree_t *at, avl_tree_node_t *atn, avl_tree_node_t *parent) {
    avl_tree_node_t *child, *grandchild;
    int8_t new_atn_balance, new_child_balance;
    int comparison, err = -1;
    
    assert(atn_is_left_heavy(atn));
    
    child = atn->atn_lchild;
    grandchild = child->atn_rchild;
    
    // the only case in which child is perfectly balanced is when it has no children
    assert(!atn_is_perfectly_balanced(child) || atn_has_no_children(child));
    
    // move the nodes to their correct new positions:
    
    if (!parent) { // parent == root
        assert(atn == at->at_root);
        at->at_root = child;
    } else {
        comparison = at->at_ops->ato_compare_fn(atn->atn_data, parent->atn_data);
        assert(comparison);
        if (comparison > 0) // right side of our parent
            parent->atn_rchild = child;
        else // left side of our parent
            parent->atn_lchild = child;
    }
    child->atn_rchild = atn;
    atn->atn_lchild = grandchild;
    
    // now adjust the balances of the nodes as necessary:
    
    new_atn_balance = atn->atn_balance + 1;
    
    if (atn_is_left_heavy(child)) {
        new_atn_balance++;
        if (atn_unbalanced(child)) {
            //
            // only way this can happen is if there was a pre-rotation
            // left during the balancing of atn. if we're balancing atn,
            // it must be unbalanced
            //
            assert(atn_unbalanced(atn));
            new_atn_balance++;
        }
        new_child_balance = 0;
        if (!atn_unbalanced(atn))
            new_child_balance++;
    } else {
        //
        // we only allow right heavy or perfectly balanced children
        // on right rotations in the case that we require the extra
        // rotation in balancing. atn must be balanced in this case
        //
        assert(!atn_unbalanced(atn) && (new_atn_balance == 0));
        new_child_balance = child->atn_balance + 1; // child may now be unbalanced
    }
    
    atn->atn_balance = new_atn_balance;
    child->atn_balance = new_child_balance;
    
    return 0;
    
error_out:
    return err;
}

static int at_rotate_left(avl_tree_t *at, avl_tree_node_t *atn, avl_tree_node_t *parent) {
    avl_tree_node_t *child, *grandchild;
    int8_t new_atn_balance, new_child_balance;
    int comparison, err = -1;
    
    assert(atn_is_right_heavy(atn));
    
    child = atn->atn_rchild;
    grandchild = child->atn_lchild;
    
    // the only case in which child is perfectly balanced is when it has no children
    assert(!atn_is_perfectly_balanced(child) || atn_has_no_children(child));
    
    // move the nodes to their correct new positions:
    
    if (!parent) { // parent == root
        assert(atn == at->at_root);
        at->at_root = child;
    } else {
        comparison = at->at_ops->ato_compare_fn(atn->atn_data, parent->atn_data);
        assert(comparison);
        if (comparison > 0) // right side of our parent
            parent->atn_rchild = child;
        else // left side of our parent
            parent->atn_lchild = child;
    }
    child->atn_lchild = atn;
    atn->atn_rchild = grandchild;
    
    // now adjust the balances of the nodes as necessary:
    
    new_atn_balance = atn->atn_balance - 1;
    
    if (atn_is_right_heavy(child)) {
        new_atn_balance--;
        if (atn_unbalanced(child)) {
            //
            // only way this can happen is if there was a pre-rotation
            // left during the balancing of atn. if we're balancing atn,
            // it must be unbalanced
            //
            assert(atn_unbalanced(atn));
            new_atn_balance--;
        }
        new_child_balance = 0;
        if (!atn_unbalanced(atn))
            new_child_balance--;
    } else {
        //
        // we only allow right heavy or perfectly balanced children
        // on right rotations in the case that we require the extra
        // rotation in balancing. atn must be balanced in this case
        //
        assert(!atn_unbalanced(atn) && (new_atn_balance == 0));
        new_child_balance = child->atn_balance - 1; // child may now be unbalanced
    }
    
    atn->atn_balance = new_atn_balance;
    child->atn_balance = new_child_balance;
    
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
        assert(!atn_unbalanced(child) && !atn_is_perfectly_balanced(child));
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
        assert(!atn_unbalanced(child) && !atn_is_perfectly_balanced(child));
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
    
    assert(!atn_unbalanced(atn));
    
    return 0;
    
error_out:
    return err;
}

static int _at_insert(avl_tree_t *at, avl_tree_node_t *atn, avl_tree_node_t *parent, avl_tree_node_t *to_insert, bool *height_change) {
    bool rheight_change = false, lheight_change = false;
    int comparison, err = -1;
    
    comparison = at->at_ops->ato_compare_fn(to_insert->atn_data, atn->atn_data);
    
    if (comparison == 0) {
        err = EEXIST;
        goto error_out;
    } else if (comparison > 0) { // insert on right
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

    //at_dump(at);
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
    
    printf("node @ %p: height %d atn_balance %d atn_lchild %p atn_rchild %p atn_data ", atn, *height, atn->atn_balance, lchild, rchild);
    at->at_ops->ato_dump_data_fn(atn->atn_data);
    printf("\n");
    
    return;
}

void at_dump(avl_tree_t *at) {
    int height;
    
    printf("at @ %p: at_nnodes %d\n", at, at->at_nnodes);
    _at_dump(at, at->at_root, &height);
    printf("\n");
}
