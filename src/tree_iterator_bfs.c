#include <vqueue.h>
#include <tree_T.h>
#include <tree_iterator.h>
#include <tree_iterator_bfs.h>

#include <stddef.h>
#include <stdint.h>

struct tree_node *tree_iterator_bfs_next(struct tree_iterator *iterator) {
    Tree_node *node;
    if(iterator == NULL || iterator->marker != TREE_ITERATOR_BFS) {
        return NULL;
    }

    if(vqueue_dequeue(iterator->node_queue, &node) != 0) {
        return NULL;
    }

    if(node == NULL) {
        return NULL;
    }

    if(node->lchild != NULL) {
        if(vqueue_enqueue(iterator->node_queue, &(node->lchild), false) != 0) {
            return NULL;
        }
    }

    if(node->rchild != NULL) {
        if(vqueue_enqueue(iterator->node_queue, &(node->rchild), false) != 0) {
            return NULL;
        }
    }

    return node;
}
