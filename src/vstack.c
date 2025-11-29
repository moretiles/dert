#include <vstack.h>
#include <pointerarith.h>

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

Vstack *vstack_create(size_t elem_size, size_t num_elems) {
    Vstack *ret;
    void *memory;

    memory = calloc(1, vstack_advise(elem_size, num_elems));
    if(memory == NULL) {
        return NULL;
    }

    if(vstack_init(&ret, memory, elem_size, num_elems) != 0) {
        free(memory);
        return NULL;
    }
    return ret;
}

size_t vstack_advise(size_t elem_size, size_t num_elems) {
    return (1 * sizeof(Vstack)) +
           (elem_size * num_elems);
}

size_t vstack_advisev(size_t num_stacks, size_t elem_size, size_t num_elems) {
    return num_stacks * ((1 * sizeof(Vstack)) +
                         (elem_size * num_elems));
}

int vstack_init(Vstack **dest, void *memory, size_t elem_size, size_t num_elems) {
    Vstack *stack;

    if(dest == NULL || memory == NULL || elem_size == 0 || num_elems == 0) {
        return 1;
    }

    stack = memory;
    stack->elems = pointer_literal_addition(stack, 1 * sizeof(Vstack));
    stack->elem_size = elem_size;
    stack->stored = 0;
    stack->cap = num_elems;

    *dest = stack;
    return 0;
}

int vstack_initv(size_t num_queues, Vstack *dest[], void *memory, size_t elem_size, size_t num_elems) {
    Vstack *stacks;
    void *ptr;

    if(dest == NULL || memory == NULL || elem_size == 0 || num_elems == 0) {
        return 1;
    }

    stacks = memory;
    ptr = pointer_literal_addition(stacks, num_queues * sizeof(Vstack));
    for(size_t i = 0; i < num_queues; i++) {
        stacks[i].elems = ptr;
        stacks[i].elem_size = elem_size;
        stacks[i].stored = 0;
        stacks[i].cap = num_elems;
        ptr = pointer_literal_addition(ptr, elem_size * num_elems);
    }

    *dest = stacks;
    return 0;
}

int vstack_deinit(Vstack *stack) {
    if(stack == NULL || stack->elems == NULL) {
        return 0;
    }

    memset(stack, 0, sizeof(Vstack));
    return 0;
}

int vstack_destroy(Vstack *stack) {
    if(stack == NULL) {
        return 2;
    }

    vstack_deinit(stack);
    free(stack);
    return 0;
}

int vstack_push(Vstack *stack, void *src) {
    void *pushed;
    if(stack == NULL || stack->elems == NULL || src == NULL) {
        return 1;
    }

    if(stack->elem_size == 0 || stack->stored >= stack->cap) {
        return 2;
    }

    pushed = pointer_literal_addition(stack->elems, stack->elem_size * stack->stored);
    if(memcpy(pushed, src, stack->elem_size) != pushed) {
        return 3;
    }
    stack->stored++;
    return 0;
}

int vstack_pop(Vstack *stack, void *dest) {
    if(stack == NULL || stack->elems == NULL || dest == NULL) {
        return 1;
    }

    if(stack->elem_size == 0 || stack->stored == 0) {
        return 2;
    }

    if(vstack_top(stack, dest) != 0) {
        return 3;
    }
    stack->stored--;
    return 0;
}

int vstack_top(Vstack *stack, void *dest) {
    void *top;
    if(stack == NULL || stack->elems == NULL || dest == NULL) {
        return 1;
    }

    if(stack->elem_size == 0 || stack->stored == 0) {
        return 2;
    }

    top = vstack_top_direct(stack);
    if(top == NULL) {
        return 3;
    }
    if(memcpy(dest, top, stack->elem_size) != dest) {
        return 4;
    }
    return 0;
}

void *vstack_top_direct(Vstack *stack) {
    if(stack == NULL || stack->elems == NULL) {
        return NULL;
    }

    if(stack->elem_size == 0 || stack->stored == 0) {
        return NULL;
    }

    return pointer_literal_addition(stack->elems, stack->elem_size * (stack->stored - 1));
}

size_t vstack_len(Vstack *stack) {
    if(stack == NULL) {
        return 0;
    }

    return stack->stored;
}

size_t vstack_cap(Vstack *stack) {
    if(stack == NULL) {
        return 0;
    }

    return stack->cap;
}
