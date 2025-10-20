/*
 * varray_priv.h -- Variable length for an arbitrary type
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

#include <stdlib.h>
#include <stdint.h>

// Append increase new elements to the end of the Varray.
int varray_grow(Varray *array, size_t increase);

// Remove decreate existing elements from the end of the Varray.
int varray_shrink(Varray *array, size_t decrease);
