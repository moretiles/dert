/*
 * vdll_priv.h -- Doubly linked list for an arbitrary type
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

#include <stddef.h>
#include <stdbool.h>

typedef struct vdll_node {
    void *data;
    struct vdll_node *prev;
    struct vdll_node *next;
} Vdll_node;

/*
 * Allocates memory for and initializes a Vdll_node.
 * Memory for node is zeroed out and then initialized.
 * If init is NULL then no initialization performed after zeroing out.
 */
Vdll_node *vdll_node_create(size_t elem_size, int (*init)(void *arg));

/*
 * Destroys a Vdll_node that was allocated by vdll_node_create.
 * Please, only use with memory allocated by vdll_node_create!
 */
int vdll_node_destroy(Vdll_node *node, size_t elem_size, int (*deinit)(void *arg));

/*
 * Seek to specific positon in linked list.
 * Returns to original position on failure.
 */
int vdll_seek(Vdll *dll, size_t pos);

/*
 * Seek forward in linked list.
 */
int vdll_wind(Vdll *dll, size_t increment);

/*
 * Seek backward in linked list.
 */
int vdll_rewind(Vdll *dll, size_t increment);

// Control whether to insert before or after
int _vdll_insert(Vdll *dll, size_t pos, size_t num_elems, bool after);
