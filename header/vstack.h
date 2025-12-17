/*
 * vstack.h -- Stack containing elements of an arbitrary type
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef struct vstack {
    // elements that are on the stack placed here
    void *elems;

    // size of individual elements
    size_t elem_size;

    // number of elements on the stack
    size_t stored;
    
    // total number of elements this stack can store, if empty
    size_t cap;
} Vstack;

// Allocates memory for and initializes a Vstack.
Vstack *vstack_create(size_t elem_size, size_t num_elems);

// Advise how much memory a Vstack with num_elems elements each of elem_size bytes needs.
// Assumes that the same valuew for elem_size and num_elems are used when calling vstack_init
size_t vstack_advise(size_t elem_size, size_t num_elems);

// Advise for many
size_t vstack_advisev(size_t num_stacks, size_t elem_size, size_t num_elems);

// Initializes a Vstack.
int vstack_init(Vstack **dest, void *memory, size_t elem_size, size_t num_elems);

// Initialize for many
int vstack_initv(size_t num_stacks, Vstack *dest[], void *memory, size_t elem_size, size_t num_elems);

// Deinitializes a Vstack.
int vstack_deinit(Vstack *stack);

/*
 * Destroys a Vstack that was allocated by vstack_create.
 * Please, only use with memory allocated by vstack_create!
 */
int vstack_destroy(Vstack *stack);

// Pushes contents of src into stack.
int vstack_push(Vstack *stack, void *src);

// Pops top of stack into dest.
int vstack_pop(Vstack *stack, void *dest);

// Gets the contents of the top of the stack.
int vstack_top(Vstack *stack, void *dest);

/*
 * Get the memory address of the element at the top of the stack.
 * Should be used carefully as further pop operations can overwrite this memory.
 * Thus, storing the pointer returned from this across pop operations almost always introduces a bug!
 */
void *vstack_top_direct(Vstack *stack);

/*
 * Returns current number of items pushed to the stack.
 * Decreases when elements are popped.
 */
size_t vstack_len(Vstack *stack);

/*
 * Returns the total number of elements that can be pushed if the stack is empty.
 * Calculated as number of elements the stack was told to allocate when creating/initializing.
 */
size_t vstack_cap(Vstack *stack);
