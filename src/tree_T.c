// idea: implement first in generic form then with macros that take in type
// bargain-bin C++ templates, in other words
//#define src_tree_T(NAME, TYPE, COMPARISON_FUNCTION)
//#define header_tree_T(NAME, TYPE)
// Would be a good idea to wrap functions in #ifdef
// This way a function providing a custom implementation of, for example, insertion could be defined ahead of the macro expansion of src_tree_T without requiring code itself to be updated

#include <tree_T.h>
#include <tree_iterator.h>
#include <pointerarith.h>

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

// Standard API
Tree_tree *tree_create(size_t max_nodes) {
    Tree_tree *tree;
    if(max_nodes == 0) {
        return NULL;
    }

    tree = malloc(tree_advise(max_nodes));
    if(tree == NULL) {
        return NULL;
    }

    if(tree_init(&tree, tree, max_nodes) != 0) {
        free(tree);
        tree = NULL;
        return NULL;
    }

    return tree;
}

size_t tree_advise(size_t max_nodes) {
    return (1 * (sizeof(Tree_tree)) +
            (max_nodes * sizeof(Tree_node)));
}

int tree_init(Tree_tree **dest, void *memory, size_t max_nodes) {
    Tree_tree *tree;
    if(dest == NULL || memory == NULL || max_nodes == 0) {
        return EINVAL;
    }

    tree = memory;

    tree->root = NULL;
    tree->nodes = pointer_literal_addition(memory, sizeof(Tree_tree));
    tree->num_nodes = 0;
    tree->max_nodes = max_nodes;
    // other fields that may be required for iteration

    *dest = tree;
    return 0;
}

void tree_deinit(Tree_tree *tree) {
    if(tree == NULL) {
        return;
    }

    tree->root = NULL;
    tree->nodes = NULL;
    tree->num_nodes = 0;
    tree->max_nodes = 0;
    // other fields that may be required for iteration

    return;
}

void tree_destroy(Tree_tree *tree) {
    if(tree == NULL) {
        return;
    }

    tree_deinit(tree);
    free(tree);

    return;
}

bool tree_reachable(struct tree_tree *tree) {
    if(tree == NULL) {
        return false;
    }

    return tree_node_reachable(tree, tree->root);
}

struct tree_node *tree_node_reachable(struct tree_tree *tree, struct tree_node *node) {
    bool lchild_has_this_as_parent, rchild_has_this_as_parent;
    struct tree_node *lchild, *rchild;

    if(tree == NULL || node == NULL) {
        return NULL;
    }

    lchild_has_this_as_parent = true;
    lchild = node->lchild;
    if(node->lchild != NULL) {
        lchild_has_this_as_parent = (node->lchild->parent == node);
        lchild = tree_node_reachable(tree, node->lchild);
    }
    rchild_has_this_as_parent = true;
    rchild = node->rchild;
    if(node->rchild != NULL) {
        rchild_has_this_as_parent = (node->rchild->parent == node);
        rchild = tree_node_reachable(tree, node->rchild);
    }

    const bool lchild_correct = lchild_has_this_as_parent && (lchild == node->lchild);
    const bool rchild_correct = rchild_has_this_as_parent && (rchild == node->rchild);

    if(lchild_correct && rchild_correct) {
        return node;
    } else {
        return NULL;
    }
}

bool tree_ordered(struct tree_tree *tree) {
    return (tree_min(tree) != NULL) && (tree_max(tree) != NULL);
}

int tree_compare(uint8_t val1, uint8_t val2) {
    return (int) (val1 - val2);
}

// Insert new node
int tree_insert(Tree_tree *tree, uint8_t val) {
    if(tree == NULL) {
        return EINVAL;
    }

    if(tree->num_nodes == tree->max_nodes) {
        return EINVAL;
    }

    Tree_node *parent = NULL;
    Tree_node *current = tree->root;
    if(tree->root != NULL) {
        bool done = false, less;
        Tree_node *next;
        while(!done && current != NULL) {
            less = tree_compare(val, current->val) < 0;

            if(less) {
                next = current->lchild;
            } else {
                next = current->rchild;
            }

            // have found node with NULL child
            if(next == NULL) {
                done = true;
            }

            parent = current;
            current = next;
        }
    }

    const int tree_node_insert_res = tree_node_insert(tree, val, parent);
    if(tree_node_insert_res != 0) {
        return tree_node_insert_res;
    }

    return 0;
}

// tmp
int tree_insert_many_compare(const void *a, const void *b) {
    uint8_t A, B;
    if(a == NULL || b == NULL) {
        return 0;
    }

    A = *((uint8_t *) a);
    B = *((uint8_t *) b);

    return A - B;
}

struct tree_insert_many_psuedonode {
    size_t position;
    size_t length;
};

int tree_insert_many(struct tree_tree *tree, uint8_t *vals, size_t num_vals, size_t val_size, bool ordered) {
    int res = 0;
    Vqueue *queue = NULL;
    if(tree == NULL || vals == NULL || num_vals == 0 || val_size == 0) {
        return EINVAL;
    }

    // assumption is that tree is sorted in some way
    if(ordered) {
        if(num_vals == 1) {
            // already sorted
        }

        bool already_sorted = true;
        for(size_t i = 1; i < num_vals; i++) {
            if(tree_compare(vals[i - 1], vals[i]) > 0) {
                already_sorted = false;
                break;
            }
        }
        if(!already_sorted) {
            qsort(vals, num_vals, val_size, tree_insert_many_compare);
        }

        // TODO: Do in batches
        if(num_vals > 10000) {
            return EINVAL;
        }
        // should use alloca instead
        struct tree_insert_many_psuedonode node = { .position = num_vals / 2, .length = num_vals };
        queue = vqueue_create(sizeof(struct tree_insert_many_psuedonode), num_vals);
        if(queue == NULL) {
            return ENOMEM;
        }
        vqueue_enqueue(queue, &node, false);
        while(vqueue_dequeue(queue, &node) == 0) {
            const int tree_insert_res = tree_insert(tree, vals[node.position]);
            if(tree_insert_res != 0) {
                res = tree_insert_res;
                goto tree_insert_many_end;
            }

            size_t lower_len = (node.length) / 2;
            size_t lower_pos = (lower_len / 2);
            size_t upper_len = node.length - lower_len - 1;
            size_t upper_pos = lower_len + 1 + (upper_len / 2);

            if(lower_len == 0) {
                // do nothing
            } else {
                node = (struct tree_insert_many_psuedonode) {
                    .position = lower_pos, .length = lower_len
                };
                vqueue_enqueue(queue, &node, false);
            }

            if(upper_len == 0) {
                // do nothing
            } else {
                node = (struct tree_insert_many_psuedonode) {
                    .position = upper_pos, .length = upper_len
                };
                vqueue_enqueue(queue, &node, false);
            }
        }
    } else {
        for(size_t i = 0; i < num_vals; i++) {
            tree_insert(tree, vals[i]);
        }
    }

tree_insert_many_end:
    if(queue != NULL) {
        vqueue_destroy(queue);
    }
    return res;
}

int tree_node_insert(Tree_tree *tree, uint8_t val, Tree_node *expected_parent) {
    int ret = 0;
    Tree_node *new_node;
    if(tree == NULL) {
        return EINVAL;
    }

    const int tree_harvest_response = tree_harvest(tree, &new_node);
    if(tree_harvest_response != 0) {
        ret = tree_harvest_response;
        goto tree_node_insert_end;
    }
    tree_node_init(new_node, val);

    if(expected_parent == NULL) {
        if(tree->root != NULL) {
            ret = ENOTRECOVERABLE;
            goto tree_node_insert_end;
        }

        tree->root = new_node;
    } else if(expected_parent == NULL) {
        ret = ENOTRECOVERABLE;
        goto tree_node_insert_end;
    } else {
        bool less = tree_compare(val, expected_parent->val) < 0;
        if(less) {
            expected_parent->lchild = new_node;
        } else {
            expected_parent->rchild = new_node;
        }
    }
    new_node->parent = expected_parent;

tree_node_insert_end:
    if(ret != 0 && new_node != NULL) {
        tree_reap(tree, new_node);
    }

    return ret;
}

// Report if node exists
bool tree_exists(Tree_tree *tree, uint8_t val) {
    Tree_node *node;
    if(tree == NULL) {
        return false;
    }

    node = tree_lookup(tree, val);

    return (node != NULL) ? true : false;
}

// Return the first node with this value
Tree_node *tree_lookup(Tree_tree *tree, uint8_t val) {
    if(tree == NULL) {
        return NULL;
    }

    int comparison;
    bool done = false;
    Tree_node *current = tree->root, *next;
    while(!done && current != NULL) {
        comparison = tree_compare(val, current->val);

        if(comparison == 0) {
            //found it
            done = true;
        } else if(comparison < 0) {
            next = current->lchild;
        } else {
            next = current->rchild;
        }

        if(!done) {
            current = next;
        }
    }

    return current;
}

struct tree_node *tree_lookup_range(
    struct tree_tree *tree, uint8_t minimum, uint8_t maximum,
    enum tree_lookup_strategy strategy
) {
    struct tree_node *current, *target;
    bool best = false;

    if(tree == NULL) {
        return NULL;
    }

    current = tree->root;
    target = NULL;
    while(current != NULL && (target == NULL || best == false)) {
        const int comparison_min = tree_compare(minimum, current->val);
        const int comparison_max = tree_compare(maximum, current->val);
        const bool current_less_than_minimum = comparison_min > 0;
        const bool current_greater_than_maximum = comparison_max < 0;

        if(current_less_than_minimum) {
            current = current->rchild;
        } else if(current_greater_than_maximum) {
            current = current->lchild;
        } else {
            target = current;
            switch(strategy) {
            case TREE_LOOKUP_FIRST:
                best = true;
                break;
            case TREE_LOOKUP_SMALLEST:
                current = current->lchild;
                break;
            case TREE_LOOKUP_LARGEST:
                current = current->rchild;
                break;
            default:
                return NULL;
                break;
            }
        }
    }

    return target;
}

// Update the node with old_val to have new_val
// Will only update the first occurance of old_val
int tree_update(Tree_tree *tree, uint8_t new_val, uint8_t old_val) {
    if(tree == NULL) {
        return EINVAL;
    }

    const int deletion_res = tree_delete(tree, old_val);
    if(deletion_res != 0) {
        return deletion_res;
    }
    const int insertion_res = tree_insert(tree, new_val);
    if(insertion_res != 0) {
        // Insertion should always succeed if deletion succeeds
        return ENOTRECOVERABLE;
    }

    return 0;
}

int tree_update_many(struct tree_tree *tree, uint8_t *new_vals, uint8_t *old_vals, size_t num_vals, bool ordered) {
    int res;
    if(tree == NULL || new_vals == NULL || old_vals == NULL || num_vals == 0) {
        return EINVAL;
    }

    res = tree_delete_many(tree, old_vals, num_vals);
    if(res != 0) {
        return res;
    }
    res = tree_insert_many(tree, new_vals, num_vals, sizeof(uint8_t), ordered);
    if(res != 0) {
        return res;
    }

    return res;
}

// Delete elements from tree
int tree_delete(Tree_tree *tree, uint8_t val) {
    if(tree == NULL) {
        return EINVAL;
    }

    if(tree->num_nodes == 0) {
        return EINVAL;
    }

    int comparison_difference;
    bool done = false;
    Tree_node *current = tree->root, *next;
    while(!done && current != NULL) {
        comparison_difference = tree_compare(val, current->val);

        if(comparison_difference == 0) {
            //found it
            done = true;
        } else if(comparison_difference < 0) {
            next = current->lchild;
        } else {
            next = current->rchild;
        }

        if(!done) {
            current = next;
        }
    }

    const int tree_node_delete_res = tree_node_delete(tree, val, current);
    if(tree_node_delete_res != 0) {
        return tree_node_delete_res;
    }

    return 0;
}

int tree_delete_many(struct tree_tree *tree, uint8_t *vals, size_t num_vals) {
    if(tree == NULL || vals == NULL || num_vals == 0) {
        return EINVAL;
    }

    for(size_t i = 0; i < num_vals; i++) {
        const int tree_delete_res = tree_delete(tree, vals[i]);
        if(tree_delete_res != 0) {
            return tree_delete_res;
        }
    }

    return 0;
}

int tree_node_delete(Tree_tree *tree, uint8_t val, Tree_node *expected_dest) {
    // I suppose some implementation might assign some significance to expected_dest being NULL.
    // Not me though.
    if(tree == NULL || expected_dest == NULL) {
        return EINVAL;
    }

    // Ignore unused warnings for val
    (void)val;

    const int tree_reorder_move_res = tree_reorder_move(tree, expected_dest);
    if(tree_reorder_move_res != 0) {
        return tree_reorder_move_res;
    }

    tree_reap(tree, expected_dest);

    return 0;
}

int tree_reorder_move(Tree_tree *tree, Tree_node *will_be_moved) {
    Tree_node *parent;
    bool rotate_left_required = false;
    if(tree == NULL || will_be_moved == NULL) {
        return EINVAL;
    }

    if(will_be_moved->parent == NULL) {
        // Must be the root node or tree is in a state of error
        if(tree->root != will_be_moved) {
            return ENOTRECOVERABLE;
        }

        // Prepare for rotate left reorder
        if(will_be_moved->lchild != NULL && will_be_moved->rchild != NULL) {
            rotate_left_required = true;
        } else if(will_be_moved->lchild != NULL) {
            will_be_moved->lchild->parent = NULL;
            tree->root = will_be_moved->lchild;
        } else if(will_be_moved->rchild != NULL) {
            will_be_moved->rchild->parent = NULL;
            tree->root = will_be_moved->rchild;
        } else {
            tree->root = NULL;
        }
    } else if(will_be_moved->rchild == NULL && will_be_moved->lchild == NULL) {
        // Go to parent and delete this child
        parent = will_be_moved->parent;

        if(parent->lchild == will_be_moved) {
            parent->lchild = NULL;
        } else {
            parent->rchild = NULL;
        }
    } else if(will_be_moved->lchild == NULL) {
        // Using this node's only existant child, the rchild node, replace it as a child of its parent
        parent = will_be_moved->parent;

        if(parent->lchild == will_be_moved) {
            parent->lchild = will_be_moved->rchild;
            parent->lchild->parent = parent;
        } else {
            parent->rchild = will_be_moved->rchild;
            parent->rchild->parent = parent->rchild;
        }
    } else if(will_be_moved->rchild == NULL) {
        // Using this node's only existant child, the lchild node, replace it as a child of its parent
        parent = will_be_moved->parent;

        if(parent->lchild == will_be_moved) {
            parent->lchild = will_be_moved->lchild;
            parent->lchild->parent = parent;
        } else {
            parent->rchild = will_be_moved->rchild;
            parent->rchild->parent = parent;
        }
    } else {
        // Prepare for rotate left reorder
        rotate_left_required = true;
    }

    if(rotate_left_required) {
        const int tree_reorder_rol_res = tree_reorder_rol(tree, will_be_moved->lchild, will_be_moved->rchild);
        if(tree_reorder_rol_res != 0) {
            return ENOTRECOVERABLE;
        }
        if(will_be_moved->parent == NULL) {
            tree->root = will_be_moved->rchild;
            tree->root->parent = NULL;
        } else {
            const bool less = tree_compare(will_be_moved->rchild->val, will_be_moved->parent->val) < 0;
            if(less) {
                will_be_moved->parent->lchild = will_be_moved->rchild;
                will_be_moved->parent->lchild->parent = will_be_moved->parent;
            } else {
                will_be_moved->parent->rchild = will_be_moved->rchild;
                will_be_moved->parent->rchild->parent = will_be_moved->parent;
            }
        }
    }

    return 0;
}

int tree_reorder_rol(Tree_tree *tree, Tree_node *lchild, Tree_node *rchild) {
    // Inserts the left child of rchild and all of its children under lchild then rotates
    Tree_node *lchild_lchild, *lchild_rchild, *rchild_lchild, *rchild_rchild;

    // lchild_lchild and rchild_rchild not modified during this
    (void)lchild_lchild;
    (void)rchild_rchild;

    if(tree == NULL || lchild == NULL || rchild == NULL) {
        return EINVAL;
    }
    lchild_lchild = lchild->lchild;
    lchild_rchild = lchild->rchild;
    rchild_lchild = rchild->lchild;
    rchild_rchild = rchild->rchild;

    rchild->lchild = lchild;
    if(rchild->lchild != NULL) {
        rchild->lchild->parent = rchild;
    }

    // find maximum child under lchild and add rchild_lchild as its rchild
    Tree_node *current = lchild_rchild, *parent = lchild;
    while(current != NULL) {
        parent = current;
        current = current->rchild;
    }

    parent->rchild = rchild_lchild;
    if(parent->rchild != NULL) {
        parent->rchild->parent = parent;
    }

    return 0;
}

int tree_reorder_ror(Tree_tree *tree, Tree_node *lchild, Tree_node *rchild) {
    // Inserts the right child of lchild and all of its children under rchild then rotates
    Tree_node *lchild_lchild, *lchild_rchild, *rchild_lchild, *rchild_rchild;

    // lchild_lchild and rchild_rchild not modified during this
    (void)lchild_lchild;
    (void)rchild_rchild;

    if(tree == NULL || lchild == NULL || rchild == NULL) {
        return EINVAL;
    }
    lchild_lchild = lchild->lchild;
    lchild_rchild = lchild->rchild;
    rchild_lchild = rchild->lchild;
    rchild_rchild = rchild->rchild;

    lchild->rchild = rchild;

    // find minimum child under rchild and add lchild_rchild as its lchild
    Tree_node *current = rchild_lchild, *parent = rchild;
    while(current != NULL) {
        parent = current;
        current = current->lchild;
    }

    parent->lchild = lchild_rchild;

    return 0;
}

// Get new node from tree
int tree_harvest(Tree_tree *tree, Tree_node **dest) {
    // TODO:
    // actual tree_harvest implementation
    if(tree == NULL || dest == NULL) {
        return EINVAL;
    } else if(tree->num_nodes == tree->max_nodes) {
        return ENOMEM;
    }

    *dest = &(tree->nodes[tree->num_nodes]);
    tree->num_nodes++;

    return 0;
}

// Delete node from tree and put back into the pool of unused nodes
void tree_reap(Tree_tree *tree, Tree_node *node) {
    // TODO:
    // actual tree_reap implementation
    (void)tree;
    (void)node;

    memset(node, 0, sizeof(struct tree_node));

    return;
}

// Initialize node
void tree_node_init(Tree_node *node, uint8_t val) {
    memset(node, 0, sizeof(Tree_node));
    node->val = val;

    return;
}

// Deinitialize node
void tree_node_deinit(Tree_node *node) {
    memset(node, 0, sizeof(Tree_node));

    return;
}

struct tree_node *tree_max(struct tree_tree *tree) {
    return tree_node_max(tree, tree->root);
}

struct tree_node *tree_min(struct tree_tree *tree) {
    return tree_node_min(tree, tree->root);
}

struct tree_node *tree_node_max(struct tree_tree *tree, struct tree_node *node) {
    struct tree_node *max_node, *max_lchild, *max_rchild;

    (void)tree;

    if(node == NULL) {
        return NULL;
    }

    max_node = node;
    max_lchild = tree_node_max(tree, node->lchild);
    max_rchild = tree_node_max(tree, node->rchild);

    if(max_lchild != NULL) {
        if(tree_compare(max_lchild->val, node->val) >= 0) {
            // means that tree is not sorted or sorting order has been broken
            return NULL;
        }
    } else if(node->lchild != NULL) {
        // there must be an error if (max_lchild == NULL) && (node->lchild != NULL)
        return NULL;
    }

    if(max_rchild != NULL) {
        if(tree_compare(node->val, max_rchild->val) > 0) {
            // means that tree is not sorted or sorting order has been broken
            return NULL;
        }
        max_node = max_rchild;
    } else if(node->rchild != NULL) {
        // there must be an error if (max_rchild == NULL) && (node->rchild != NULL)
        return NULL;
    }

    return max_node;
}

struct tree_node *tree_node_min(struct tree_tree *tree, struct tree_node *node) {
    struct tree_node *min_node, *min_lchild, *min_rchild;

    (void)tree;

    if(node == NULL) {
        return NULL;
    }

    min_node = node;
    min_lchild = tree_node_min(tree, node->lchild);
    min_rchild = tree_node_min(tree, node->rchild);
    if(min_lchild != NULL) {
        if(tree_compare(min_lchild->val, node->val) >= 0) {
            // means that tree is not sorted or sorting order has been broken
            return NULL;
        }
        min_node = min_lchild;
    } else if(node->lchild != NULL) {
        // there must be an error if (min_lchild == NULL) && (node->lchild != NULL)
        return NULL;
    }

    if(min_rchild != NULL) {
        if(tree_compare(node->val, min_rchild->val) > 0) {
            // means that tree is not sorted or sorting order has been broken
            return NULL;
        }
    } else if(node->rchild != NULL) {
        // there must be an error if (min_rchild == NULL) && (node->rchild != NULL)
        return NULL;
    }

    return min_node;
}
