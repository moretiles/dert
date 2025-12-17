#include <vqueue.h>
#include <vqueue_priv.h>
#include <pointerarith.h>

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

size_t vqueue_wrap(Vqueue *queue, size_t pos) {
    if(queue == NULL) {
        return 0;
    }

    return pos % queue->cap;
}

Vqueue *vqueue_create(size_t elem_size, size_t num_elems) {
    Vqueue *ret;

    if(elem_size == 0 || num_elems == 0) {
        return NULL;
    }

    ret = malloc(vqueue_advise(elem_size, num_elems));
    if(ret == NULL) {
        return NULL;
    }

    if(vqueue_init(ret, elem_size, num_elems) != 0) {
        free(ret);
        return NULL;
    }
    return ret;
}

size_t vqueue_advise(size_t elem_size, size_t num_elems) {
    return 1 * ((1 * sizeof(Vqueue)) +
                (num_elems * elem_size));
}

size_t vqueue_advisev(size_t num_queues, size_t elem_size, size_t num_elems) {
    return num_queues * ((1 * sizeof(Vqueue)) +
                         (num_elems * elem_size));
}

int vqueue_init(Vqueue *queue, size_t elem_size, size_t num_elems) {
    if(queue == NULL || elem_size == 0 || num_elems == 0) {
        return EINVAL;
    }

    if(memset(queue, 0, vqueue_advise(elem_size, num_elems)) != queue){
        return ENOTRECOVERABLE;
    }
    queue->elems = pointer_literal_addition(queue, sizeof(Vqueue));
    queue->elem_size = elem_size;
    queue->front = 0;
    queue->back = 0;
    queue->cap = num_elems;
    return 0;
}

int vqueue_initv(size_t num_queues, Vqueue *dest[], void *memory, size_t elem_size, size_t num_elems) {
    Vqueue *queue;
    if(num_queues == 0 || dest == NULL || memory == NULL || elem_size == 0 || num_elems == 0) {
        return 1;
    }

    queue = memory;
    if(memset(queue, 0, vqueue_advisev(num_queues, elem_size, num_elems)) != queue){
        return ENOTRECOVERABLE;
    }
    for(size_t i = 0; i < num_queues; i++) {
        queue->elems = pointer_literal_addition(queue, sizeof(Vqueue));
        memset(queue->elems, 0, elem_size * num_elems);
        queue->elem_size = elem_size;
        queue->front = 0;
        queue->back = 0;
        queue->cap = num_elems;
        queue = pointer_literal_addition(queue, vqueue_advise(elem_size, num_elems));
    }

    *dest = memory;
    return 0;
}

void vqueue_deinit(Vqueue *queue) {
    if(queue == NULL || queue->elems == NULL) {
        return;
    }

    memset(queue, 0, vqueue_advise(queue->elem_size, queue->cap));
    return;
}

void vqueue_destroy(Vqueue *queue) {
    if(queue == NULL) {
        return;
    }

    vqueue_deinit(queue);
    free(queue);
    return;
}

int vqueue_enqueue(Vqueue *queue, void *src, bool overwrite) {
    void *enqueued;
    if(queue == NULL || src == NULL) {
        return EINVAL;
    }

    // check if full
    if(!overwrite && !(queue->front == queue->back && queue->front == 0 && queue->back == 0) && vqueue_wrap(queue, queue->front) == vqueue_wrap(queue, queue->back)) {
        return EXFULL;
    }

    enqueued = pointer_literal_addition(queue->elems, queue->elem_size * vqueue_wrap(queue, queue->back));
    if(memcpy(enqueued, src, queue->elem_size) != enqueued) {
        return ENOTRECOVERABLE;
    }
    queue->back++;

    // only can occur if overwrite is true, need to shift front because we just overwrote something
    if(queue->back > queue->cap) {
        queue->front++;
    }

    if(queue->front > vqueue_wrap(queue, queue->front) && queue->back > vqueue_wrap(queue, queue->back)) {
        queue->front = vqueue_wrap(queue, queue->front);
        queue->back = vqueue_wrap(queue, queue->back);
    }

    return 0;
}

int vqueue_dequeue(Vqueue *queue, void *dest) {
    if(queue == NULL) {
        return EINVAL;
    }

    if(vqueue_front(queue, dest) != 0) {
        return ENODATA;
    }
    queue->front++;

    if(queue->front > vqueue_wrap(queue, queue->front) && queue->back > vqueue_wrap(queue, queue->back)) {
        queue->front = vqueue_wrap(queue, queue->front);
        queue->back = vqueue_wrap(queue, queue->back);
    }

    if(queue->front == queue->back) {
        queue->front = 0;
        queue->back = 0;
    }

    return 0;
}

int vqueue_front(Vqueue *queue, void *dest) {
    void *front;

    if(queue == NULL || dest == NULL) {
        return EINVAL;
    }

    //if(vqueue_wrap(queue, queue->front) == vqueue_wrap(queue, queue->back)){
    if(queue->back == 0) {
        return ENODATA;
    }

    front = vqueue_front_direct(queue);
    if(front == NULL) {
        return ENODATA;
    }
    if(memcpy(dest, front, queue->elem_size) != dest) {
        return ENOTRECOVERABLE;
    }
    return 0;
}

void *vqueue_front_direct(Vqueue *queue) {
    if(queue == NULL) {
        return NULL;
    }

    //if(vqueue_wrap(queue, queue->front) == vqueue_wrap(queue, queue->back)){
    if(queue->back == 0) {
        return NULL;
    }

    return pointer_literal_addition(queue->elems, queue->elem_size * vqueue_wrap(queue, queue->front));
}

int vqueue_back(Vqueue *queue, void *dest) {
    void *back;

    if(queue == NULL) {
        return EINVAL;
    }

    //if(vqueue_wrap(queue, queue->front) == vqueue_wrap(queue, queue->back)){
    if(queue->back == 0) {
        return ENODATA;
    }

    back = vqueue_back_direct(queue);
    if(back == NULL) {
        return ENODATA;
    }
    if(memcpy(dest, back, queue->elem_size) != dest) {
        return ENOTRECOVERABLE;
    }
    return 0;
}

void *vqueue_back_direct(Vqueue *queue) {
    if(queue == NULL) {
        return NULL;
    }

    //if(vqueue_wrap(queue, queue->front) == vqueue_wrap(queue, queue->back)){
    if(queue->back == 0) {
        return NULL;
    }

    return pointer_literal_addition(queue->elems, queue->elem_size * vqueue_wrap(queue, queue->back - 1));
}

size_t vqueue_len(Vqueue *queue) {
    if(queue == NULL) {
        return EINVAL;
    }

    return queue->back - queue->front;
}

size_t vqueue_cap(Vqueue *queue) {
    if(queue == NULL) {
        return EINVAL;
    }

    return queue->cap;
}
