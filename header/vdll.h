/*
 * dll.h -- Vdll and Vdll_functions
 *
 * c_misc - Some useful C files I wrote
 * https://github.com/moretiles/c_misc
 * Project licensed under Apache-2.0 license
 */

#include <stddef.h>

/*
 * User defined functions for allocating and deallocating individual elements
 */
typedef struct vdll_functions {
	int (*init)(void *arg);
	int (*deinit)(void *arg);
} Vdll_functions;

typedef struct vdll {
    // Ptr to node at current position.
	struct vdll_node *ptr;

    // Size of individual elements.
	size_t elem_size;

    // Current position into linked list.
	size_t pos;
    
    // Total number of elements stored in linked list.
	size_t cap;

    // Functions used for initializing and deinitializing elements.
	struct vdll_functions *functions;
} Vdll;

// Allocates memory for and initializes a Vdll
Vdll *vdll_create(size_t elem_size, Vdll_functions *functions);

// Initializes a Vdll
int vdll_init(Vdll *dll, size_t elem_size, Vdll_functions *functions);

// Denitializes a Vdll
void vdll_deinit(Vdll *dll);

/*
 * Destroys a Vdll that was allocated by vdll_create.
 * Please, only use with memory allocated by vdll_create!
 */
void vdll_destroy(Vdll *dll);

// Copy the contents of the element at pos in dll to dest.
int vdll_get(Vdll *dll, size_t pos, void *dest);

/*
 * Get the memory address of the element at pos in dll.
 * Should be used carefully as further delete/shrink operations can destroy associated memory.
 * Thus, storing the pointer returned from this across delete/shrink operations almost always introduces a bug!
 */
void *vdll_get_direct(Vdll *dll, size_t pos);

// Copy the contents of src to the element at pos in dll.
int vdll_set(Vdll *dll, size_t pos, void *src);

// Get current number of elements stored in dll.
size_t vdll_len(Vdll *dll);

// Create num_elems new elements shifting elements at pos to after the new elements.
int vdll_insert(Vdll *dll, size_t pos, size_t num_elems);

// Delete elements from pos to (pos + num_elems - 1) in dll.
int vdll_delete(Vdll *dll, size_t pos, size_t num_elems);

// Create num_elems new elements and insert them after the current final element.
int vdll_grow(Vdll *dll, size_t num_elems);

// Remove the num_elems last elements from dll.
int vdll_shrink(Vdll *dll, size_t num_elems);
