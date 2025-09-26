#include <stddef.h>
#include <stdbool.h>

typedef struct vqueue {
    // a little of bit of pointer arithmetic trickery
    //void *contents

    size_t elem_size;

    // front
    size_t front;

    // back
    size_t back;

    size_t cap;
} Vqueue;

Vqueue *vqueue_init(size_t elem_size, size_t num_elems);
int vqueue_destroy(Vqueue **queue_ptr);

int vqueue_enqueue(Vqueue **queue_ptr, void *elem, bool overwrite);
void *vqueue_dequeue(Vqueue **queue_ptr);

void *vqueue_front(Vqueue **queue_ptr);
void *vqueue_back(Vqueue **queue_ptr);
