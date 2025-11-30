#include <mpscqueue.h>
#include <mpscqueue_priv.h>
#include <pointerarith.h>

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>

// This implementation is nearly identical to aqueue
// Main difference is that in order to enqueue producers lock a mutex

size_t mpscqueue_wrap(Mpscqueue *queue, size_t pos) {
    if(queue == NULL) {
        return 0;
    }

    return pos % queue->cap;
}

Mpscqueue *mpscqueue_create(size_t elem_size, size_t num_elems) {
    Mpscqueue *ret;

    if(elem_size == 0 || num_elems == 0) {
        return NULL;
    }

    ret = calloc(1, sizeof(Mpscqueue));
    if(ret == NULL) {
        return NULL;
    }

    if(mpscqueue_init(ret, elem_size, num_elems) != 0) {
        free(ret);
        return NULL;
    }
    return ret;
}

int mpscqueue_init(Mpscqueue *queue, size_t elem_size, size_t num_elems) {
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
    queue->cap = num_elems;
    pthread_mutex_init(&(queue->producer_mutex), NULL);
    atomic_store_explicit(&(queue->len), 0, memory_order_release);
    return 0;
}

void mpscqueue_deinit(Mpscqueue *queue) {
    if(queue == NULL || queue->elems == NULL) {
        return;
    }

    free(queue->elems);
    pthread_mutex_destroy(&(queue->producer_mutex));
    atomic_store_explicit(&(queue->len), 0, memory_order_release);
    memset(queue, 0, sizeof(Mpscqueue));
    return;
}

void mpscqueue_destroy(Mpscqueue *queue) {
    if(queue == NULL) {
        return;
    }

    mpscqueue_deinit(queue);
    free(queue);
    return;
}

int mpscqueue_enqueue(Mpscqueue *queue, void *src) {
    void *enqueued;

    if(queue == NULL || src == NULL) {
        return 1;
    }

    if(pthread_mutex_lock(&(queue->producer_mutex)) != 0) {
        return 2;
    }

    // fail if full
    if(atomic_load_explicit(&(queue->len), memory_order_acquire) == queue->cap) {
        return 3;
    }

    enqueued = pointer_literal_addition(queue->elems, queue->elem_size * mpscqueue_wrap(queue, queue->back));
    if(memcpy(enqueued, src, queue->elem_size) != enqueued) {
        return 4;
    }
    queue->back = mpscqueue_wrap(queue, queue->back + 1);
    atomic_fetch_add_explicit(&(queue->len), 1LU, memory_order_release);

    if(pthread_mutex_unlock(&(queue->producer_mutex)) != 0) {
        return 5;
    }

    return 0;
}

int mpscqueue_dequeue(Mpscqueue *queue, void *dest) {
    if(queue == NULL) {
        return 0;
    }

    // fails when queue->len == 0
    if(mpscqueue_front(queue, dest) != 0) {
        return 1;
    }
    queue->front = mpscqueue_wrap(queue, queue->front + 1);
    atomic_fetch_sub_explicit(&(queue->len), 1LU, memory_order_release);

    return 0;
}

int mpscqueue_front(Mpscqueue *queue, void *dest) {
    void *front;

    if(queue == NULL) {
        return 1;
    }

    if(atomic_load_explicit(&(queue->len), memory_order_acquire) == 0) {
        return 2;
    }

    front = mpscqueue_front_direct(queue);
    if(front == NULL) {
        return 3;
    }
    if(memcpy(dest, front, queue->elem_size) != dest) {
        return 4;
    }
    return 0;
}

void *mpscqueue_front_direct(Mpscqueue *queue) {
    if(queue == NULL) {
        return NULL;
    }

    if(atomic_load_explicit(&(queue->len), memory_order_acquire) == 0) {
        return NULL;
    }

    return pointer_literal_addition(queue->elems, queue->elem_size * mpscqueue_wrap(queue, queue->front));
}

size_t mpscqueue_len(Mpscqueue *queue) {
    if(queue == NULL) {
        return 0;
    }

    return atomic_load_explicit(&(queue->len), memory_order_acquire);
}

size_t mpscqueue_cap(Mpscqueue *queue) {
    if(queue == NULL) {
        return 0;
    }

    return queue->cap;
}
