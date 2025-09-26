#include <stddef.h>
#include <stdbool.h>

typedef struct vstack {
    // a little of bit of pointer arithmetic trickery
    //void *contents
    size_t elem_size;
    size_t stored;
    size_t cap;
} Vstack;

Vstack *vstack_init(size_t elem_size, size_t num_elems);
int vstack_destroy(Vstack **stack_ptr);

int vstack_push(Vstack **stack_ptr, void *elem);
void *vstack_pop(Vstack **stack_ptr);

void *vstack_top(Vstack **stack_ptr);
