// idea: implement first in generic form then with macros that take in type
// bargain-bin C++ templates, in other words
//#define src_tree_T(NAME, TYPE, COMPARISON_FUNCTION)
//#define header_tree_T(NAME, TYPE)
// Would be a good idea to wrap functions in #ifdef
// This way a function providing a custom implementation of, for example, insertion could be defined ahead of the macro expansion of src_tree_T without requiring code itself to be updated

#pragma once

#include <vqueue.h>

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Node inside tree
typedef struct tree_node {
    // value being stored
    uint8_t val;

    // This node's immediate parent
    struct tree_node *parent;

    // The left child
    struct tree_node *lchild;

    // The right child
    struct tree_node *rchild;
} Tree_node;

// Tree of Tree_Nodes
typedef struct tree_tree {
    // The root of the tree
    struct tree_node *root;

    // Nodes themselves
    struct tree_node *nodes;

    // Current number of alive children in tree
    size_t num_nodes;

    // Total number of nodes that can be placed on the tree
    size_t max_nodes;
} Tree_tree;

// Standard API
struct tree_tree *tree_create(size_t max_nodes);
size_t tree_advise(size_t max_nodes);
int tree_init(struct tree_tree **dest, void *memory, size_t max_nodes);
void tree_deinit(struct tree_tree *tree);
void tree_destroy(struct tree_tree *tree);

// comparator function
int tree_compare(uint8_t val1, uint8_t val2);

// Return the first node with this value
struct tree_node *tree_lookup(struct tree_tree *tree, uint8_t val);

enum tree_lookup_strategy {
    TREE_LOOKUP_FIRST,
    TREE_LOOKUP_SMALLEST,
    TREE_LOOKUP_LARGEST
};

// Return the best node from this range given a strategy
struct tree_node *tree_lookup_range(
        struct tree_tree *tree, uint8_t minimum, uint8_t maximum,
        enum tree_lookup_strategy strategy
    );

// Get maximum node below some node in tree
struct tree_node *tree_max(struct tree_tree *tree);

// Get minimum node below some node in tree
struct tree_node *tree_min(struct tree_tree *tree);

// Get maximum node below some node in tree
struct tree_node *tree_node_max(struct tree_tree *tree, struct tree_node *node);

// Get minimum node below some node in tree
struct tree_node *tree_node_min(struct tree_tree *tree, struct tree_node *node);

// check to make sure all elements on tree connect both ways
bool tree_reachable(struct tree_tree *tree);

// returns address of node if all of its children can reach it
struct tree_node *tree_node_reachable(struct tree_tree *tree, struct tree_node *node);

// check to make sure tree is correctly ordered
// some trees may not use comparative ordering in this way
bool tree_ordered(struct tree_tree *tree);

// Initialize node
void tree_node_init(struct tree_node *node, uint8_t val);

// Deinitialize node
void tree_node_deinit(struct tree_node *node);

// Insert new node
int tree_insert(struct tree_tree *tree, uint8_t val);

// Insert many new nodes
int tree_insert_many(struct tree_tree *tree, uint8_t *vals, size_t num_vals, size_t val_size, bool ordered);

// Once the destination and parent for the inserted node have been found this function is called
// It should actually insert the node itself into the tree and do any other reordering needed
int tree_node_insert(struct tree_tree *tree, uint8_t val, struct tree_node *expected_parent);

// Report if node exists
bool tree_exists(struct tree_tree *tree, uint8_t val);

// Update the node with old_val to have new_val
// May cause reordering of tree
// Deltion callback then insertion callback invoked as part of this
int tree_update(struct tree_tree *tree, uint8_t new_val, uint8_t old_val);

// Update many existing nodes
int tree_update_many(struct tree_tree *tree, uint8_t *new_vals, uint8_t *old_vals, size_t num_vals, bool ordered);

// Delete elements from tree
int tree_delete(struct tree_tree *tree, uint8_t val);

// Delete many existing nodes
int tree_delete_many(struct tree_tree *tree, uint8_t *vals, size_t num_vals);

// Once the destination node (i.e. first node with val) is found this function is called
// It should actually delete the node itself and do any other reordering needed
int tree_node_delete(struct tree_tree *tree, uint8_t val, struct tree_node *expected_dest);

// Decide how to handle reordering tree after node will_be_moved is moved
// Deleting a node is also considered "moving" it so called then
int tree_reorder_move(struct tree_tree *tree, struct tree_node *will_be_moved);

// Do rotate-left of tree making rchild parent of lchild
int tree_reorder_rol(struct tree_tree *tree, struct tree_node *lchild, struct tree_node *rchild);

// Do rotate-right of tree making lchild parent of rchild
int tree_reorder_ror(struct tree_tree *tree, struct tree_node *lchild, struct tree_node *rchild);

// Get new node from tree
int tree_harvest(struct tree_tree *tree, struct tree_node **dest);

// Delete node from tree and put back into the pool of unused nodes
void tree_reap(struct tree_tree *tree, struct tree_node *node);
