#pragma once

#include <vqueue.h>

#include <stddef.h>
#include <stdint.h>

enum tree_iterator_marker {
    TREE_ITERATOR_PRE,
    TREE_ITERATOR_IN,
    TREE_ITERATOR_POST,
    TREE_ITERATOR_BFS
};

typedef struct tree_iterator {
    // The tree itself
    struct tree_tree *tree;

    enum tree_iterator_marker marker;

    size_t num_visited;

    size_t max_nodes;

    union {
        // used by pre-order, in-order, and post-order search
        struct {
            struct tree_node *prev;
            struct tree_node *current;
            struct tree_node *next;
        };

        // used by BFS search
        struct vqueue *node_queue;
    };
} Tree_iterator;

// Standard API
Tree_iterator *tree_iterator_create(struct tree_tree *tree, enum tree_iterator_marker marker, size_t max_nodes);
size_t tree_iterator_advise(struct tree_tree *tree, enum tree_iterator_marker marker, size_t max_nodes);
int tree_iterator_init(
        Tree_iterator **dest, void *memory,
        struct tree_tree *tree, enum tree_iterator_marker marker, size_t max_nodes
    );
void tree_iterator_deinit(Tree_iterator *iterator);
void tree_iterator_destroy(Tree_iterator *iterator);

// Get the next element from the iterator
struct tree_node *tree_iterator_next(Tree_iterator *iterator);

// Print all nodes iterated through over the course of the tree to terminal
// Optionally, produce .gv file that can be used with graphviz dot command describing structure
int tree_puts(
        struct tree_tree *tree, Tree_iterator *iterator,
        int ((*stringify)(char *dest, struct tree_node *src, size_t cap)), char *filename
    );

// Allocates a pre-order iterator, calls tree_puts, then deallocates the iterator
int tree_puts_pre(
        struct tree_tree *tree,
        int ((*stringify)(char *dest, struct tree_node *src, size_t cap)), char *filename
    );

// Allocates a in-order iterator, calls tree_puts, then deallocates the iterator
int tree_puts_in(
        struct tree_tree *tree,
        int ((*stringify)(char *dest, struct tree_node *src, size_t cap)), char *filename
    );

// Allocates a post-order iterator, calls tree_puts, then deallocates the iterator
int tree_puts_post(
        struct tree_tree *tree,
        int ((*stringify)(char *dest, struct tree_node *src, size_t cap)), char *filename
    );

// Allocates a breadth first search iterator, calls tree_puts, then deallocates the iterator
int tree_puts_bfs(
        struct tree_tree *tree,
        int ((*stringify)(char *dest, struct tree_node *src, size_t cap)), char *filename
    );
