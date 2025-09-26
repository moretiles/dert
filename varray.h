#include <stddef.h>

typedef struct varray {
    void *contents;
    size_t elem_size;
    size_t stored;
    size_t cap;
} Varray;

Varray *varray_init(size_t elem_size);
int varray_destroy(Varray **array_ptr);

void *varray_get(Varray **array_ptr, size_t pos);
int varray_set(Varray **array_ptr, size_t pos, void *src);

int varray_grow(Varray **array_ptr, size_t increase);
int varray_shrink(Varray **array_ptr, size_t decrease);

size_t varray_len(Varray *array);
size_t varray_cap(Varray *array);
