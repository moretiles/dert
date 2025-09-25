#include "varray.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

Varray *varray_init(size_t elem_size){
    if(elem_size == 0){
        return NULL;
    }

    Varray *ret = calloc(1, sizeof(Varray));
    ret->contents = NULL;
    ret->elem_size = elem_size;
    ret->stored = 0;
    ret->cap = 0;

    return ret;
}

int varray_destroy(Varray **array_ptr){
    Varray *array;

    if(array_ptr == NULL){
        return 1;
    }

    array = *array_ptr;
    if(array == NULL){
        return 0;
    }

    if(array->contents != NULL){
        free(array->contents);
        array->contents = NULL;
    }
    free(array);
    *array_ptr = NULL;

    return 0;
}

void *varray_get(Varray **array_ptr, size_t pos){
    Varray *array;
    
    if(array_ptr == NULL){
        return NULL;
    }

    array = *array_ptr;
    if(array == NULL){
        return NULL;
    }

    if(pos >= array->stored){
        return NULL;
    }

    return array->contents + (pos * array->elem_size);
}

int varray_set(Varray **array_ptr, size_t pos, void *src){
    void *dest = varray_get(array_ptr, pos);

    if(dest == NULL){
        return 1;
    }

    if(memcpy(dest, src, (*array_ptr)->elem_size) != dest){
        return 2;
    }

    return 0;
}

int varray_grow(Varray **array_ptr, size_t increase){
    Varray *array;

    if(array_ptr == NULL){
        return 1;
    }
    
    array = *array_ptr;
    if(array == NULL){
        return 2;
    }

    if(array->contents == NULL){
        array->contents = calloc(increase, array->elem_size);
        if(array->contents == NULL){
            return 3;
        }

        array->stored += increase;
        array->cap += increase;
    } else {
        size_t new_cap = (array->cap << 1) | 0x1;
        while(new_cap < (array->cap + increase)){
            new_cap <<= 1;
        }

        void *new_contents = calloc(new_cap, array->elem_size);
        if(new_contents == NULL){
            return 4;
        }

        if(memcpy(new_contents, array->contents, array->cap * array->elem_size) != new_contents){
            return 5;
        }
        free(array->contents);
        array->contents = new_contents;
        array->stored += increase;
        array->cap = new_cap;
    }

    return 0;
}

int varray_shrink(Varray **array_ptr, size_t decrease){
    Varray *array;

    if(array_ptr == NULL){
        return 1;
    }
    
    array = *array_ptr;
    if(array == NULL){
        return 2;
    }

    if(array->cap < decrease){
        return 3;
    }

    void *dest = realloc(array->contents, (array->cap - decrease) * array->elem_size);
    if(dest == NULL){
        return 4;
    }
    array->contents = dest;
    array->cap -= decrease;
    if(array->cap < array->stored){
        array->stored = array->cap;
    }

    return 0;
}
