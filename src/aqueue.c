#include <aqueue.h>
#include <aqueue_priv.h>
#include <pointerarith.h>

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

size_t aqueue_wrap(Aqueue *queue, size_t pos) {
    if(queue == NULL) {
        return 0;
    }

    return pos % queue->cap;
}

Aqueue *aqueue_create(size_t elem_size, size_t num_elems) {
    Aqueue *ret;

    if(elem_size == 0 || num_elems == 0) {
        return NULL;
    }

    ret = calloc(1, sizeof(Aqueue));
    if(ret == NULL) {
        return NULL;
    }

    if(aqueue_init(ret, elem_size, num_elems) != 0) {
        free(ret);
        return NULL;
    }
    return ret;
}

int aqueue_init(Aqueue *queue, size_t elem_size, size_t num_elems) {
    if(queue == NULL || elem_size == 0 || num_elems == 0) {
        return 1;
    }

    queue->elems = calloc(num_elems, elem_size);
    if(queue->elems == NULL) {
        return 2;
    }

    queue->elem_size = elem_size;
    queue->front = 0;
    queue->back = 0;
    queue->len = 0;
    queue->cap = num_elems;
    return 0;
}

void aqueue_deinit(Aqueue *queue) {
    if(queue == NULL || queue->elems == NULL) {
        return;
    }

    free(queue->elems);
    memset(queue, 0, sizeof(Aqueue));
    return;
}

void aqueue_destroy(Aqueue *queue) {
    if(queue == NULL) {
        return;
    }

    aqueue_deinit(queue);
    free(queue);
    return;
}

int aqueue_enqueue(Aqueue *queue, void *src) {
    void *enqueued;

    if(queue == NULL || src == NULL) {
        return 1;
    }

    // fail if full
    if(queue->len == queue->cap) {
        return 3;
    }

    enqueued = pointer_literal_addition(queue->elems, queue->elem_size * aqueue_wrap(queue, queue->back));
    if(memcpy(enqueued, src, queue->elem_size) != enqueued) {
        return 4;
    }
    queue->back = aqueue_wrap(queue, queue->back + 1);
    __atomic_add_fetch(&(queue->len), 1LU, __ATOMIC_SEQ_CST);

    return 0;
}

int aqueue_dequeue(Aqueue *queue, void *dest) {
    if(queue == NULL) {
        return 0;
    }

    // fails when queue->len == 0
    if(aqueue_front(queue, dest) != 0) {
        return 1;
    }
    queue->front = aqueue_wrap(queue, queue->front + 1);
    __atomic_sub_fetch(&(queue->len), 1LU, __ATOMIC_SEQ_CST);

    return 0;
}

int aqueue_front(Aqueue *queue, void *dest) {
    void *front;

    if(queue == NULL) {
        return 1;
    }

    if(queue->len == 0) {
        return 2;
    }

    front = aqueue_front_direct(queue);
    if(front == NULL) {
        return 3;
    }
    if(memcpy(dest, front, queue->elem_size) != dest) {
        return 4;
    }
    return 0;
}

void *aqueue_front_direct(Aqueue *queue) {
    if(queue == NULL) {
        return NULL;
    }

    if(queue->len == 0) {
        return NULL;
    }

    return pointer_literal_addition(queue->elems, queue->elem_size * aqueue_wrap(queue, queue->front));
}

size_t aqueue_len(Aqueue *queue) {
    if(queue == NULL) {
        return 0;
    }

    return queue->len;
}

size_t aqueue_cap(Aqueue *queue) {
    if(queue == NULL) {
        return 0;
    }

    return queue->cap;
}
