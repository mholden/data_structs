#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "avl-tree.h"

avl_tree_t *at_create(avl_tree_ops_t *ato) {
    avl_tree_t *at;
    
    at = (avl_tree_t *)malloc(sizeof(avl_tree_t));
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

static void _atn_destroy(avl_tree_t *at, avl_tree_node_t *atn) {
    if (at->at_ops->ato_destroy_data_fn)
        at->at_ops->ato_destroy_data_fn(atn->atn_data);
    free(atn);
}

static void _at_destroy(avl_tree_t *at, avl_tree_node_t *atn) {
    if (!atn) return;
    _at_destroy(at, atn->atn_lchild);
    _at_destroy(at, atn->atn_rchild);
    _atn_destroy(at, atn);
}

void at_destroy(avl_tree_t *at) {
    _at_destroy(at, at->at_root);
    free(at);
    return;
}

static void _at_check(avl_tree_t *at, avl_tree_node_t *atn, int *height, size_t *nnodes) {
    avl_tree_node_t *rchild, *lchild;
    int rheight = 0, lheight = 0, sub_height, balance;
    
    if (!atn) return;
    
    rchild = atn->atn_rchild;
    lchild = atn->atn_lchild;
    if (rchild) {
        assert(at->at_ops->ato_compare_fn(rchild->atn_data, atn->atn_data) > 0);
        _at_check(at, rchild, &rheight, nnodes);
    }
    if (lchild) {
        assert(at->at_ops->ato_compare_fn(lchild->atn_data, atn->atn_data) < 0);
        _at_check(at, lchild, &lheight, nnodes);
    }
    
    balance = rheight - lheight;
    assert((atn->atn_balance == balance) && (abs(balance) <= 1));
    sub_height = (lheight > rheight) ? lheight : rheight;
    if (height)
        *height = sub_height + 1;
    if (nnodes)
        (*nnodes)++;
    
    return;
}

void at_check(avl_tree_t *at) {
    size_t nnodes = 0;
    _at_check(at, at->at_root, NULL, &nnodes);
    assert(nnodes == at->at_nnodes);
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

static bool atn_is_child_of(avl_tree_node_t *atn, avl_tree_node_t *potential_parent) {
    if ((atn == potential_parent->atn_lchild) || (atn == potential_parent->atn_rchild))
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
    
    atn = (avl_tree_node_t *)malloc(sizeof(avl_tree_node_t));
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

static int _at_find(avl_tree_t *at, avl_tree_node_t *atn, void *to_find, void **data) {
    int comparison;
    
    if (!atn) return ENOENT;
    
    comparison = at->at_ops->ato_compare_fn(to_find, atn->atn_data);
    if (comparison == 0) { // found it
        if (data)
            *data = atn->atn_data;
    } else if (comparison > 0)
        return _at_find(at, atn->atn_rchild, to_find, data);
    else // < 0
        return _at_find(at, atn->atn_lchild, to_find, data);
    
    return 0;
}

int at_find(avl_tree_t *at, void *to_find, void **data) {
    return _at_find(at, at->at_root, to_find, data);
}

static void at_get_max_for_removal(avl_tree_t *at, avl_tree_node_t *atn, avl_tree_node_t *parent, avl_tree_node_t **max, bool *height_change) {
    bool rheight_change = false;
    
    if (!atn->atn_rchild) { // this is the max; remove it
        if (at->at_ops->ato_compare_fn(atn->atn_data, parent->atn_data) > 0)
            parent->atn_rchild = atn->atn_lchild;
        else
            parent->atn_lchild = atn->atn_lchild;
        *height_change = true;
        *max = atn;
        goto out;
    } else {
        at_get_max_for_removal(at, atn->atn_rchild, atn, max, &rheight_change);
    }
    
    if (rheight_change) {
        if (atn_is_right_heavy(atn))
            *height_change = true;
        atn->atn_balance--;
    }
    
    if (atn_unbalanced(atn)) {
        assert(rheight_change);
        //
        // similar to the _at_remove case, if we're unbalanced it
        // means we were left heavy. so *height_change should not
        // have been set above
        //
        assert(*height_change == false);
        assert(atn_is_left_heavy(atn));
        
        // similar to _at_remove, too:
        if (atn_is_perfectly_balanced(atn->atn_lchild))
            *height_change = false;
        else
            *height_change = true;
        
        at_balance(at, atn, parent);
    }
    
out:
    return;
}

static void at_get_min_for_removal(avl_tree_t *at, avl_tree_node_t *atn, avl_tree_node_t *parent, avl_tree_node_t **min, bool *height_change) {
    bool lheight_change = false;
    
    if (!atn->atn_lchild) { // this is the min; remove it
        if (at->at_ops->ato_compare_fn(atn->atn_data, parent->atn_data) > 0)
            parent->atn_rchild = atn->atn_rchild;
        else
            parent->atn_lchild = atn->atn_rchild;
        *height_change = true;
        *min = atn;
        goto out;
    } else {
        at_get_max_for_removal(at, atn->atn_lchild, atn, min, &lheight_change);
    }
    
    if (lheight_change) {
        if (atn_is_left_heavy(atn))
            *height_change = true;
        atn->atn_balance++;
    }
    
    if (atn_unbalanced(atn)) {
        assert(lheight_change);
        //
        // similar to the _at_remove case, if we're unbalanced it
        // means we were left heavy. so *height_change should not
        // have been set above
        //
        assert(*height_change == false);
        assert(atn_is_right_heavy(atn));
        
        // similar to _at_remove, too:
        if (atn_is_perfectly_balanced(atn->atn_rchild))
            *height_change = false;
        else
            *height_change = true;
        
        at_balance(at, atn, parent);
    }
    
out:
    return;
}

static void at_get_replacement_for_removal(avl_tree_t *at, avl_tree_node_t *atn, avl_tree_node_t **replacement, bool *lheight_change, bool *rheight_change) {
    assert(!atn_has_no_children(atn));
    if (atn->atn_lchild)
        at_get_max_for_removal(at, atn->atn_lchild, atn, replacement, lheight_change);
    else
        at_get_min_for_removal(at, atn->atn_rchild, atn, replacement, rheight_change);
}

static int _at_remove(avl_tree_t *at, avl_tree_node_t *atn, avl_tree_node_t *parent, void *to_remove, bool *height_change) {
    avl_tree_node_t *replacement;
    bool rheight_change = false, lheight_change = false;
    int comparison, err = -1;
    
    if (!atn) return ENOENT;
    
    comparison = at->at_ops->ato_compare_fn(to_remove, atn->atn_data);
    
    if (comparison == 0) { // found it, remove it
        if (atn_has_no_children(atn)) { // leaf node
            if (!parent) { // root node case
                at->at_root = NULL;
            } else {
                if (at->at_ops->ato_compare_fn(atn->atn_data, parent->atn_data) > 0)
                    parent->atn_rchild = NULL;
                else
                    parent->atn_lchild = NULL;
            }
            if (height_change)
                *height_change = true;
            _at_destroy(at, atn);
            goto out;
        } else {
            //
            // similar to classic bst algorithm, grab the max of the left subtree or
            // the min of the right subtree, and replace the node we're removing with
            // that. main difference is that we need to track height changes and
            // rebalance subtrees if necessary along the way
            //
            at_get_replacement_for_removal(at, atn, &replacement, &lheight_change, &rheight_change);
            
            // at this point, replacement has been plucked (removed) from the tree. now swap atn out with it
            
            // do the swap
            if (!parent) { // root node case
                at->at_root = replacement;
            } else {
                if (at->at_ops->ato_compare_fn(atn->atn_data, parent->atn_data) > 0)
                    parent->atn_rchild = replacement;
                else
                    parent->atn_lchild = replacement;
            }
            
            //
            // replacement shouldn't be a child of atn at this point. replacement
            // shouldn't be a child of any node at this point, actually
            //
            assert(!atn_is_child_of(replacement, atn));
            
            replacement->atn_lchild = atn->atn_lchild;
            replacement->atn_rchild = atn->atn_rchild;
            replacement->atn_balance = atn->atn_balance;
            
            _atn_destroy(at, atn);
            atn = replacement;
        }
    } else if (comparison > 0) {
        err = _at_remove(at, atn->atn_rchild, atn, to_remove, &rheight_change);
        if (err)
            goto error_out;
    } else { // < 0
        err = _at_remove(at, atn->atn_lchild, atn, to_remove, &lheight_change);
        if (err)
            goto error_out;
    }
    
    if (rheight_change) {
        if (atn_is_right_heavy(atn)) {
            //
            // height change of our right subtree changes
            // our height only if we're right heavy
            //
            if (height_change)
                *height_change = true;
        }
        atn->atn_balance--;
    } else if (lheight_change) {
        if (atn_is_left_heavy(atn)) { // same thing on the left side
            if (height_change)
                *height_change = true;
        }
        atn->atn_balance++;
    }
    
    if (atn_unbalanced(atn)) {
        assert(rheight_change || lheight_change);
        //
        // if we're unbalanced, it means either there's been an rheight change
        // on a left-heavy tree or a lheight change on a right-heavy tree. in
        // both those cases, *height_change should not have been set to true
        // above
        //
        assert((height_change == NULL) || (*height_change == false));
        //
        // unlike the insert case, balancing might actually not
        // reduce the height of the tree. this (balancing not
        // reducing the height of the tree) only happens when
        // the side that we're heavy on is perfectly balanced
        //
        if ((atn_is_right_heavy(atn) && atn_is_perfectly_balanced(atn->atn_rchild)) ||
            (atn_is_left_heavy(atn) && atn_is_perfectly_balanced(atn->atn_lchild))) {
            if (height_change)
                *height_change = false;
        } else {
            if (height_change)
                *height_change = true;
        }
        at_balance(at, atn, parent);
    }
    
out:
    return 0;
    
error_out:
    return err;
}

int at_remove(avl_tree_t *at, void *data) {
    int err = -1;
    
    err = _at_remove(at, at->at_root, NULL, data, NULL);
    if (err)
        goto error_out;
    
    at->at_nnodes--;
    
    //at_check(at);

    return 0;
    
error_out:
    return err;
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
    
    printf("node @ %p: height %d atn_balance %d atn_lchild %p atn_rchild %p atn_data @ %p ", atn, *height, atn->atn_balance, lchild, rchild, atn->atn_data);
    if (at->at_ops->ato_dump_data_fn)
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
