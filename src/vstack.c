#include <vstack.h>
#include <pointerarith.h>

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

Vstack *vstack_create(size_t elem_size, size_t num_elems) {
    Vstack *ret = calloc(1, sizeof(Vstack));
    if(ret == NULL){
        return NULL;
    }

    if(vstack_init(ret, elem_size, num_elems) != 0){
        free(ret);
        return NULL;
    }
    return ret;
}

int vstack_init(Vstack *stack, size_t elem_size, size_t num_elems){
    if(stack == NULL || elem_size == 0 || num_elems == 0){
        return 1;
    }

    stack->elems = calloc(num_elems, elem_size);
    if(stack->elems == NULL){
        return 2;
    }

    stack->elem_size = elem_size;
    stack->stored = 0;
    stack->cap = num_elems;
    return 0;
}

int vstack_deinit(Vstack *stack){
    if(stack == NULL || stack->elems == NULL){
        return 0;
    }

    free(stack->elems);
    memset(stack, 0, sizeof(Vstack));
    return 0;
}

int vstack_destroy(Vstack *stack){
    if(stack == NULL){
        return 2;
    }

    vstack_deinit(stack);
    free(stack);
    return 0;
}

int vstack_push(Vstack *stack, void *src){
    void *pushed;
    if(stack == NULL || stack->elems == NULL || src == NULL){
        return 1;
    }

    if(stack->elem_size == 0 || stack->stored >= stack->cap){
        return 2;
    }

    pushed = pointer_literal_addition(stack->elems, stack->elem_size * stack->stored);
    if(memcpy(pushed, src, stack->elem_size) != pushed){
        return 3;
    }
    stack->stored++;
    return 0;
}

int vstack_pop(Vstack *stack, void *dest){
    if(stack == NULL || stack->elems == NULL || dest == NULL){
        return 1;
    }

    if(stack->elem_size == 0 || stack->stored == 0){
        return 2;
    }

    if(vstack_top(stack, dest) != 0){
        return 3;
    }
    stack->stored--;
    return 0;
}

int vstack_top(Vstack *stack, void *dest){
    void *top;
    if(stack == NULL || stack->elems == NULL || dest == NULL){
        return 1;
    }

    if(stack->elem_size == 0 || stack->stored == 0){
        return 2;
    }

    top = vstack_top_direct(stack);
    if(top == NULL){
        return 3;
    }
    if(memcpy(dest, top, stack->elem_size) != dest){
        return 4;
    }
    return 0;
}

void *vstack_top_direct(Vstack *stack){
    if(stack == NULL || stack->elems == NULL){
        return NULL;
    }

    if(stack->elem_size == 0 || stack->stored == 0){
        return NULL;
    }

    return pointer_literal_addition(stack->elems, stack->elem_size * (stack->stored - 1));
}

size_t vstack_len(Vstack *stack){
    if(stack == NULL){
        return 0;
    }

    return stack->stored;
}

size_t vstack_cap(Vstack *stack){
    if(stack == NULL){
        return 0;
    }

    return stack->cap;
}
