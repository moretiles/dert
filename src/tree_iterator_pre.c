#include <errno.h>
#include <stdio.h>

#include <tree_T.h>
#include <tree_iterator.h>
#include <tree_iterator_pre.h>

Tree_node *tree_iterator_pre_find_smallest_above(Tree_node *current) {
    Tree_node *next;
    if(current == NULL || current->parent == NULL) {
        return NULL;
    }

    next = NULL;
    while(next == NULL) {
        if(current->parent == NULL) {
            // just went up from root node, iteration is complete

            next = NULL;
            break;
        } else if(current->parent->lchild == current) {
            if(current->parent->rchild != NULL) {
                // found it
                next = current->parent->rchild;
            }
        } else if(current->parent->rchild == current) {
            // need to keep going
            current = current->parent;
        } else {
            // error

            next = NULL;
            break;
        }
    }

    return next;
}

struct tree_node *tree_iterator_pre_next(struct tree_iterator *iterator) {
    struct tree_node *will_return_this;
    if(iterator == NULL) {
        return NULL;
    }

    will_return_this = iterator->next;

    iterator->prev = iterator->current;
    iterator->current = iterator->next;
    iterator->next = NULL;

    if(will_return_this == NULL) {
        // don't try to calculate next node
        return NULL;
    }

    if(iterator->current->lchild != NULL) {
        iterator->next = iterator->current->lchild;
    } else if (iterator->current->rchild != NULL) {
        iterator->next = iterator->current->rchild;
    } else {
        iterator->next = tree_iterator_pre_find_smallest_above(iterator->current);
    }

    return will_return_this;
}
