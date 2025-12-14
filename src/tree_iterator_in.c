#include <errno.h>
#include <stdio.h>

#include <tree_T.h>
#include <tree_iterator.h>
#include <tree_iterator_in.h>

Tree_node *tree_iterator_in_find_smallest_below(Tree_node *current) {
    Tree_node *next;
    if(current == NULL || current->rchild == NULL) {
        return NULL;
    }

    current = current->rchild;
    next = NULL;
    while(next == NULL) {
        if(current->lchild != NULL) {
            current = current->lchild;
        } else {
            next = current;
        }
    }

    return next;
}

Tree_node *tree_iterator_in_find_smallest_above(Tree_node *current) {
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
            // all good

            next = current->parent;
        } else if(current->parent->rchild == current) {
            // need to repeat and go to this node's parent
            current = current->parent;
            next = NULL;
        } else {
            // error

            next = NULL;
            break;
        }
    }

    return next;
}

struct tree_node *tree_iterator_in_next(struct tree_iterator *iterator) {
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

    const bool current_is_the_first_node = iterator->prev == NULL;
    const bool current_is_lchild_of_something = (
            iterator->current->parent &&
            iterator->current == iterator->current->parent->lchild
        );
    const bool current_is_rchild_of_prev = (
            iterator->current->parent != NULL &&
            iterator->prev == iterator->current->parent &&
            iterator->current == iterator->prev->rchild
                                           );
    const bool prev_is_lchild_of_current = iterator->prev == iterator->current->lchild;
    const bool current_node_and_left_path_already_iterated_over = (
            current_is_lchild_of_something ||
            current_is_rchild_of_prev ||
            prev_is_lchild_of_current
        );

    if(current_is_the_first_node) {
        iterator->next = iterator->current->parent;
    } else if(current_node_and_left_path_already_iterated_over) {
        // next largest element must be at or below rchild of current, otherwise need to go up

        if(iterator->current->rchild != NULL) {
            iterator->next = tree_iterator_in_find_smallest_below(iterator->current);
        } else {
            iterator->next = tree_iterator_in_find_smallest_above(iterator->current);
        }
    } else {
        // next largest element must be at or below rchild of current

        iterator->next = tree_iterator_in_find_smallest_below(iterator->current);
    }

    return will_return_this;
}
