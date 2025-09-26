#include "vstack.h"
#include "pointerarith.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

Vstack *vstack_init(size_t elem_size, size_t num_elems){
    Vstack *ret;

    if(elem_size == 0 || num_elems == 0){
        return NULL;
    }

    ret = calloc(1, sizeof(Vstack) + (elem_size * num_elems));
    if(ret == NULL){
        return NULL;
    }

    ret->elem_size = elem_size;
    ret->stored = 0;
    ret->cap = num_elems;
    return ret;
}

int vstack_destroy(Vstack **stack_ptr){
    Vstack *stack;
    if(stack_ptr == NULL){
        return 1;
    }

    stack = *stack_ptr;
    if(stack == NULL){
        return 2;
    }

    // pop while not empty
    while(vstack_top(&stack) != NULL){
        if(vstack_pop(&stack) != 0){
            return 3;
        }
    }

    free(stack);
    return 0;
}

int vstack_push(Vstack **stack_ptr, void *elem){
    Vstack *stack;
    void *pushed;
    if(stack_ptr == NULL || elem == NULL){
        return 1;
    }

    stack = *stack_ptr;
    if(stack == NULL){
        return 2;
    }
    
    if(stack->stored >= stack->cap){
        return 3;
    }

    pushed = pointer_literal_addition(stack, sizeof(Vstack) + (stack->elem_size * stack->stored));
    if(memcpy(pushed, elem, stack->elem_size) != pushed){
        return 4;
    }
    stack->stored++;
    return 0;
}

void *vstack_pop(Vstack **stack_ptr){
    Vstack *stack;
    void *popped;
    if(stack_ptr == NULL){
        return NULL;
    }

    stack = *stack_ptr;
    if(stack == NULL){
        return NULL;
    }

    if(stack->stored == 0){
        return NULL;
    }

    popped = vstack_top(stack_ptr);
    if(popped == NULL){
        return NULL;
    }
    stack->stored--;
    return popped;
}

void *vstack_top(Vstack **stack_ptr){
    Vstack *stack;
    void *top;
    if(stack_ptr == NULL){
        return NULL;
    }

    stack = *stack_ptr;
    if(stack == NULL){
        return NULL;
    }

    if(stack->stored == 0){
        return NULL;
    }

    top = pointer_literal_addition(stack, sizeof(Vstack) + (stack->elem_size * (stack->stored - 1)));
    if(top == NULL){
        return NULL;
    }
    return top;
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
