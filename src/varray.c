#include <varray.h>
#include <varray_priv.h>
#include <pointerarith.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

Varray *varray_create(size_t elem_size){
    if(elem_size == 0){
        return NULL;
    }

    Varray *ret = calloc(1, sizeof(Varray));
    if(ret == NULL){
        return NULL;
    }

    if(varray_init(ret, elem_size) != 0){
        free(ret);
        return NULL;
    }
    return ret;
}

int varray_init(Varray *array, size_t elem_size){
    if(array == NULL || elem_size == 0){
        return 1;
    }

    array->elems = NULL;
    array->elem_size = elem_size;
    array->stored = 0;
    array->cap = 0;
    return 0;
}

void varray_deinit(Varray *array){
    if(array == NULL){
        return;
    }

    if(array->elems != NULL){
        free(array->elems);
    }
    memset(array, 0, sizeof(Varray));

    return;
}

void varray_destroy(Varray *array){
    if(array == NULL){
        return;
    }

    varray_deinit(array);
    free(array);

    return;
}

int varray_get(Varray *array, size_t pos, void *dest){
    void *src;
    if(array == NULL || dest == NULL){
        return 1;
    }

    src = varray_get_direct(array, pos);
    if(src == NULL){
        return 2;
    }

    if(memcpy(dest, src, array->elem_size) != dest){
        return 3;
    }
    return 0;
}

void *varray_get_direct(Varray *array, size_t pos){
    if(array == NULL){
        return NULL;
    }

    if(pos >= array->stored){
        return NULL;
    }

    // pointer addition in this case scales by 1
    return pointer_literal_addition(array->elems, pos * array->elem_size);
}

int varray_set(Varray *array, size_t pos, void *src){
    void *dest = varray_get_direct(array, pos);

    if(dest == NULL){
        return 1;
    }

    if(memcpy(dest, src, array->elem_size) != dest){
        return 2;
    }

    return 0;
}

int varray_resize(Varray *array, size_t new_size){
    if(new_size == array->stored){
        return 0;
    } else if(new_size > array->stored){
        if(new_size > array->cap){
            return varray_realloc(array, new_size);
        } else {
            array->stored = new_size;
            return 0;
        }
    } else {
        array->stored = new_size;
        return 0;
    }
}

int varray_realloc(Varray *array, size_t new_size){
    if(new_size == array->cap){
        return 0;
    } else if(new_size > array->cap){
        return varray_grow(array, new_size - array->cap);
    } else {
        return varray_shrink(array, array->cap - new_size);
    }
}

int varray_grow(Varray *array, size_t increase){
    if(array == NULL){
        return 2;
    }

    if(increase == 0){
        return 3;
    }

    if(array->elems == NULL){
        array->elems = calloc(increase, array->elem_size);
        if(array->elems == NULL){
            return 4;
        }

        array->stored += increase;
        array->cap += increase;
    } else {
        size_t new_cap = (array->cap + 1);
        while(new_cap < (array->cap + increase)){
            new_cap <<= 1;
        }

        void *new_elems = realloc(array->elems, new_cap * array->elem_size);
        if(new_elems == NULL){
            return 5;
        }
        array->elems = new_elems;
        memset(pointer_literal_addition(array->elems, array->cap * array->elem_size), 0, (new_cap - array->cap) * array->elem_size);

        array->stored += increase;
        array->cap = new_cap;
    }

    return 0;
}

int varray_shrink(Varray *array, size_t decrease){
    if(array == NULL){
        return 2;
    }

    if(decrease == 0){
        return 3;
    }

    if(array->cap < decrease){
        return 4;
    }

    void *dest = realloc(array->elems, (array->cap - decrease) * array->elem_size);
    if(dest == NULL){
        return 5;
    }
    array->elems = dest;
    array->cap -= decrease;
    if(array->cap < array->stored){
        array->stored = array->cap;
    }

    return 0;
}

size_t varray_len(Varray *array){
    if(array == NULL){
        return 0;
    }

    return array->stored;
}

size_t varray_cap(Varray *array){
    if(array == NULL){
        return 0;
    }

    return array->cap;
}
