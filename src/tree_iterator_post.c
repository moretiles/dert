#include <errno.h>
#include <stdio.h>

#include <tree_T.h>
#include <tree_iterator.h>
#include <tree_iterator_post.h>

Tree_node *tree_iterator_post_find_smallest_below(Tree_node *current) {
    Tree_node *next;
    if(current == NULL) {
        return NULL;
    }

    if(current->rchild == NULL) {
        return current;
    }

    current = current->rchild;
    next = NULL;
    while(next == NULL) {
        if(current->lchild != NULL) {
            current = current->lchild;
        } else if(current->rchild != NULL) {
            current = current->rchild;
        } else {
            next = current;
        }
    }

    return next;
}

Tree_node *tree_iterator_post_find_smallest_above(Tree_node *current) {
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
            // next is either current->parent->rchild or one of its children

            next = tree_iterator_post_find_smallest_below(current->parent);
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

struct tree_node *tree_iterator_post_next(struct tree_iterator *iterator) {
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

    if(iterator->prev == NULL) {
        iterator->next = tree_iterator_post_find_smallest_above(iterator->current);
    } else if(iterator->current->parent != NULL && iterator->current == iterator->current->parent->rchild) {
        iterator->next = iterator->current->parent;
    } else if(iterator->current->parent != NULL && iterator->current == iterator->current->parent->lchild) {
        iterator->next = tree_iterator_post_find_smallest_below(iterator->current->parent);
    } else {
    }

    return will_return_this;
}
