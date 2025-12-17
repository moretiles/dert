#include <vqueue.h>
#include <tree_T.h>
#include <tree_iterator.h>
#include <tree_iterator_pre.h>
#include <tree_iterator_in.h>
#include <tree_iterator_post.h>
#include <tree_iterator_bfs.h>
#include <pointerarith.h>

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Standard API
Tree_iterator *tree_iterator_create(Tree_tree *tree, enum tree_iterator_marker marker, size_t max_nodes) {
    Tree_iterator *iterator;

    switch(marker) {
    case TREE_ITERATOR_PRE:
    case TREE_ITERATOR_IN:
    case TREE_ITERATOR_POST:
    case TREE_ITERATOR_BFS:
        // marker is valid
        break;
    default:
        return NULL;
        break;
    }

    if(tree == NULL || max_nodes == 0) {
        return NULL;
    }

    iterator = malloc(tree_iterator_advise(tree, marker, max_nodes));
    if(iterator == NULL) {
        return NULL;
    }

    const int tree_iterator_init_res = tree_iterator_init(&iterator, iterator, tree, marker, max_nodes);
    if(tree_iterator_init_res != 0) {
        free(iterator);
        iterator = NULL;
        return NULL;
    }

    return iterator;
}

size_t tree_iterator_advise(Tree_tree *tree, enum tree_iterator_marker marker, size_t max_nodes) {
    if(tree == NULL || max_nodes == 0) {
        return 0;
    }

    // TODO: Switch vqueue to using advise api so an exact amount of memory is specified upfront
    switch(marker) {
    case TREE_ITERATOR_PRE:
    case TREE_ITERATOR_IN:
    case TREE_ITERATOR_POST:
        return 1 * (sizeof(Tree_iterator));
        break;
    case TREE_ITERATOR_BFS:
        return 1 * ((sizeof(Tree_iterator)) +
                    (vqueue_advise(sizeof(struct tree_node*), max_nodes)));
        break;
    default:
        // error
        return 0;
        break;
    }

    return 0;
}

int tree_iterator_init(
    Tree_iterator **dest, void *memory,
    Tree_tree *tree, enum tree_iterator_marker marker, size_t max_nodes
) {
    Tree_iterator *iterator;

    switch(marker) {
    case TREE_ITERATOR_PRE:
    case TREE_ITERATOR_IN:
    case TREE_ITERATOR_POST:
    case TREE_ITERATOR_BFS:
        // marker is valid
        break;
    default:
        return EINVAL;
        break;
    }

    if(dest == NULL || memory == NULL || tree == NULL || max_nodes == 0) {
        return EINVAL;
    }

    iterator = memory;
    if(memset(iterator, 0, tree_iterator_advise(tree, marker, max_nodes)) != iterator) {
        return ENOTRECOVERABLE;
    }
    iterator->tree = tree;
    iterator->marker = marker;
    iterator->num_visited = 0;
    iterator->max_nodes = max_nodes;

    switch(marker) {
    case TREE_ITERATOR_PRE:
        iterator->prev = NULL;
        iterator->current = NULL;
        iterator->next = tree->root;

        break;
    case TREE_ITERATOR_IN:
        iterator->prev = NULL;
        iterator->current = NULL;
        iterator->next = tree->root;
        while(iterator->next != NULL && iterator->next->lchild != NULL) {
            iterator->next = iterator->next->lchild;
        }
        break;
    case TREE_ITERATOR_POST:
        iterator->prev = NULL;
        iterator->current = NULL;
        iterator->next = tree->root;
        while(iterator->next != NULL && iterator->next->lchild != NULL) {
            iterator->next = iterator->next->lchild;
        }
        break;
    case TREE_ITERATOR_BFS:
        iterator->node_queue = pointer_literal_addition(iterator, sizeof(Tree_iterator));
        if(vqueue_init(iterator->node_queue, sizeof(struct tree_node*), max_nodes) != 0) {
            return EINVAL;
        }
        if(vqueue_enqueue(iterator->node_queue, &(tree->root), false) != 0) {
            return EINVAL;
        }
        // enqueue everything
        break;
    default:
        return EINVAL;
        break;
    }

    *dest = memory;
    return 0;
}

void tree_iterator_deinit(Tree_iterator *iterator) {
    if(iterator == NULL) {
        return;
    }

    switch(iterator->marker) {
    case TREE_ITERATOR_PRE:
    case TREE_ITERATOR_IN:
    case TREE_ITERATOR_POST:
        iterator->prev = NULL;
        iterator->current = NULL;
        iterator->next = NULL;
        break;
    case TREE_ITERATOR_BFS:
        vqueue_deinit(iterator->node_queue);
        break;
    default:
        break;
    }

    memset(iterator, 0, tree_iterator_advise(iterator->tree, iterator->marker, iterator->max_nodes));

    return;
}

void tree_iterator_destroy(Tree_iterator *iterator) {
    if(iterator == NULL) {
        return;
    }

    tree_iterator_deinit(iterator);
    free(iterator);

    return;
}

// Get the next element from the iterator
Tree_node *tree_iterator_next(Tree_iterator *iterator) {
    Tree_node *next;
    if(iterator == NULL) {
        return NULL;
    }

    switch(iterator->marker) {
    case TREE_ITERATOR_PRE:
        next = tree_iterator_pre_next(iterator);
        break;
    case TREE_ITERATOR_IN:
        next = tree_iterator_in_next(iterator);
        break;
    case TREE_ITERATOR_POST:
        next = tree_iterator_post_next(iterator);
        // next ig
        break;
    case TREE_ITERATOR_BFS:
        next = tree_iterator_bfs_next(iterator);
        break;
    default:
        return NULL;
        break;
    }

    return next;
}

#define TREE_PUTS_BUFFER_SIZE (999)
int tree_puts(
    Tree_tree *tree, Tree_iterator *iterator,
    int ((*stringify)(char *dest, Tree_node *src, size_t cap)), char *filename
) {
    (void)filename;

    int ret = 0;
    Tree_node *node;
    char node_as_string[TREE_PUTS_BUFFER_SIZE];

    if(tree == NULL || iterator == NULL) {
        ret = EINVAL;
        goto tree_puts_end;
    }

    if(filename != NULL) {
        printf("filename: %s\n", filename);
    }
    printf("<<START\n");
    while((node = tree_iterator_next(iterator))) {
        if(stringify != NULL) {
            if(stringify(node_as_string, node, TREE_PUTS_BUFFER_SIZE) != 0) {
                strcpy(node_as_string, "FORMAT_ERROR");
            }
        } else {
            snprintf(node_as_string, TREE_PUTS_BUFFER_SIZE, "%lu", (size_t) node->val);
        }
        printf("%s, ", node_as_string);

        printf("\n");
    }
    printf(">>END\n");

    if(filename == NULL) {
        goto tree_puts_end;
    }

#ifdef DERT_TEST
#define DERT_TREE_PUTS_ARENA_CAP (5 * 1024 * 1024)
#include <varena.h>
#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>

    // You likely will not need this much memory
    Varena *nodes_arena = varena_create(DERT_TREE_PUTS_ARENA_CAP);
    assert(nodes_arena != NULL);

    // In order to produce a graph that displays correctly always use BFS
    iterator = tree_iterator_create(iterator->tree, TREE_ITERATOR_BFS, iterator->max_nodes);
    assert(iterator != NULL);

    Agraph_t *g = agopen("G", Agdirected, NULL);
    assert(g != NULL);
    GVC_t *gvc = gvContext();
    assert(gvc != NULL);
    // ParseArgs interface used to trick graphviz into thinking this is the dot command
    int pretend_argc = 4;
    char *pretend_argv[] = {"./dot", "-Tpng", "-o", filename};
    gvParseArgs(gvc, pretend_argc, pretend_argv);

    while((node = tree_iterator_next(iterator))) {
        if(stringify != NULL) {
            if(stringify(node_as_string, node, TREE_PUTS_BUFFER_SIZE) != 0) {
                strcpy(node_as_string, "FORMAT_ERROR");
            }
        } else {
            snprintf(node_as_string, TREE_PUTS_BUFFER_SIZE, "%lu", (size_t) node->val);
        }

        assert(varena_claim(&nodes_arena, strlen(node_as_string) + 1) == 0);
        char *node_as_string_in_arena = varena_alloc(&nodes_arena, strlen(node_as_string + 1));
        // always enough memory so no overflow
        assert(strcpy(node_as_string_in_arena, node_as_string) == node_as_string_in_arena);
        assert(node_as_string_in_arena != NULL);

        assert(agnode(g, node_as_string_in_arena, true) != NULL);
        if(node->parent) {
            if(stringify != NULL) {
                if(stringify(node_as_string, node->parent, TREE_PUTS_BUFFER_SIZE) != 0) {
                    strcpy(node_as_string, "FORMAT_ERROR");
                    assert(false);
                }
            } else {
                snprintf(node_as_string, TREE_PUTS_BUFFER_SIZE, "%lu", (size_t) node->parent->val);
            }

            Agnode_t *parent = agnode(g, node_as_string, false);
            Agnode_t *child = agnode(g, node_as_string_in_arena, false);
            assert(parent != NULL);
            assert(child != NULL);
            Agedge_t *edge;

            // setting color for the created edge using agsafeset requires a context and rendering engine
            if(node->parent->lchild == node) {
                edge = agedge(g, parent, child, node_as_string_in_arena, true);
                assert(edge != NULL);
                agsafeset(edge, "color", "blue", "");
            } else {
                edge = agedge(g, parent, child, node_as_string_in_arena, true);
                assert(edge != NULL);
                agsafeset(edge, "color", "orange", "");
            }
        }
    }

    // It appears that some dependencies of graphviz are not freeing memory properly
    // Total amount is trivial (< 10 KB)
    gvLayoutJobs(gvc, g);
    gvRenderJobs(gvc, g);
    gvFreeLayout(gvc, g);
    agclose(g);
    gvFinalize(gvc);
    gvFreeContext(gvc);

    varena_destroy(&nodes_arena);
    // uses the iterator allocated earlier in this ifdef block, not the one provided as an argument
    tree_iterator_destroy(iterator);
#endif

tree_puts_end:
    return ret;
}

int tree_puts_pre(
    Tree_tree *tree,
    int ((*stringify)(char *dest, Tree_node *src, size_t cap)), char *filename
) {
    Tree_iterator *iterator;
    int ret = 0;

    if(tree == NULL) {
        return EINVAL;
    }

    iterator = tree_iterator_create(tree, TREE_ITERATOR_PRE, tree->max_nodes);
    if(iterator == NULL) {
        return ENOMEM;
    }
    ret = tree_puts(tree, iterator, stringify, filename);
    tree_iterator_destroy(iterator);

    return ret;
}

int tree_puts_in(
    Tree_tree *tree,
    int ((*stringify)(char *dest, Tree_node *src, size_t cap)), char *filename
) {
    Tree_iterator *iterator;
    int ret = 0;

    if(tree == NULL) {
        return EINVAL;
    }

    iterator = tree_iterator_create(tree, TREE_ITERATOR_IN, tree->max_nodes);
    if(iterator == NULL) {
        return ENOMEM;
    }
    ret = tree_puts(tree, iterator, stringify, filename);
    tree_iterator_destroy(iterator);

    return ret;
}

int tree_puts_post(
    Tree_tree *tree,
    int ((*stringify)(char *dest, Tree_node *src, size_t cap)), char *filename
) {
    Tree_iterator *iterator;
    int ret = 0;

    if(tree == NULL) {
        return EINVAL;
    }

    iterator = tree_iterator_create(tree, TREE_ITERATOR_POST, tree->max_nodes);
    if(iterator == NULL) {
        return ENOMEM;
    }
    ret = tree_puts(tree, iterator, stringify, filename);
    tree_iterator_destroy(iterator);

    return ret;
}

int tree_puts_bfs(
    Tree_tree *tree,
    int ((*stringify)(char *dest, Tree_node *src, size_t cap)), char *filename
) {
    Tree_iterator *iterator;
    int ret = 0;

    if(tree == NULL) {
        return EINVAL;
    }

    iterator = tree_iterator_create(tree, TREE_ITERATOR_BFS, tree->max_nodes);
    if(iterator == NULL) {
        return ENOMEM;
    }
    ret = tree_puts(tree, iterator, stringify, filename);
    tree_iterator_destroy(iterator);

    return ret;
}
