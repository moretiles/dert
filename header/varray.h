/*
 * varray.h -- Variable length for an arbitrary type
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

#include <stddef.h>

typedef struct varray {
    // elements that form the Varray
    void *elems;

    // size of individual elements
    size_t elem_size;

    // number of elements the varray currently holds
    size_t stored;

    // total number of elements the Varray can (currently) support
    size_t cap;
} Varray;

// Allocates memory for and initializes an empty Varray
Varray *varray_create(size_t elem_size);

// Initializes a Varray
int varray_init(Varray *array, size_t elem_size);

// Deinitializes a Varray
void varray_deinit(Varray *array);

/*
 * Destroys a Varray that was allocated by varray_create.
 * Please, only use with memory allocated by varray_create!
 */
void varray_destroy(Varray *array);

// Copies the element at pos of array to dest.
int varray_get(Varray *array, size_t pos, void *dest);

/*
 * Get the memory address of the element at pos of array.
 * Should be used carefully as deleting elements from the Varray can overwrite this memory.
 * Thus, storing the pointer returned from this across delete operations almost always introduces a bug!
 */
void *varray_get_direct(Varray *array, size_t pos);

// Copies the element at src to pos of array.
int varray_set(Varray *array, size_t pos, void *src);

// Resize varray (does not reallocate memory)
int varray_resize(Varray *array, size_t new_size);

// Reallocate memory required for varray (does reallocate memory)
int varray_realloc(Varray *array, size_t new_size);

// Provide the current number of positions at which elements can be stored.
size_t varray_len(Varray *array);

// Provide the cap for number of elements that can currently be stored without resizing.
size_t varray_cap(Varray *array);
