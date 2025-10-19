#include <stdlib.h>
#include <stdint.h>

// Append increase new elements to the end of the Varray.
int varray_grow(Varray *array, size_t increase);

// Remove decreate existing elements from the end of the Varray.
int varray_shrink(Varray *array, size_t decrease);
