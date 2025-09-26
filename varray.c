#include "varray.h"
#include "pointerarith.h"

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

    // pointer addition in this case scales by 1
    return pointer_literal_addition(array->contents, pos * array->elem_size);
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

    if(increase == 0){
        return 3;
    }

    if(array->contents == NULL){
        array->contents = calloc(increase, array->elem_size);
        if(array->contents == NULL){
            return 4;
        }

        array->stored += increase;
        array->cap += increase;
    } else {
        size_t new_cap = (array->cap << 1);
        while(new_cap < (array->cap + increase)){
            new_cap <<= 1;
        }

        void *new_contents = calloc(new_cap, array->elem_size);
        if(new_contents == NULL){
            return 5;
        }

        if(memcpy(new_contents, array->contents, array->cap * array->elem_size) != new_contents){
            return 6;
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

    if(decrease == 0){
        return 3;
    }

    if(array->cap < decrease){
        return 4;
    }

    void *dest = realloc(array->contents, (array->cap - decrease) * array->elem_size);
    if(dest == NULL){
        return 5;
    }
    array->contents = dest;
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
