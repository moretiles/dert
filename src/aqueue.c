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
    void *memory;
    Aqueue *ret;

    if(elem_size == 0 || num_elems == 0) {
        return NULL;
    }

    memory = calloc(1, aqueue_advise(elem_size, num_elems));
    if(memory == NULL) {
        return NULL;
    }

    if(aqueue_init(&ret, memory, elem_size, num_elems) != 0) {
        free(memory);
        return NULL;
    }
    return ret;
}

size_t aqueue_advise(size_t elem_size, size_t num_elems){
    return (1 * sizeof(Aqueue)) +
           (num_elems * elem_size);
}

size_t aqueue_advisev(size_t num_queues, size_t elem_size, size_t num_elems){
    return num_queues * ((1 * sizeof(Aqueue)) +
                         (num_elems * elem_size));
}

int aqueue_init(Aqueue **dest, void *memory, size_t elem_size, size_t num_elems) {
    Aqueue *queue;
    void *ptr;

    if(dest == NULL || memory == NULL || elem_size == 0 || num_elems == 0) {
        return 1;
    }

    ptr = memory;
    ptr = pointer_literal_addition(ptr, 0);
    queue = (Aqueue *) ptr;
    ptr = pointer_literal_addition(ptr, 1 * sizeof(Aqueue));
    queue->elems = (void *) ptr;

    queue->elem_size = elem_size;
    queue->front = 0;
    queue->back = 0;
    __atomic_store_n(&(queue->len), 0, __ATOMIC_SEQ_CST);
    queue->cap = num_elems;

    *dest = queue;
    return 0;
}

int aqueue_initv(size_t num_queues, Aqueue *dest[], void *memory, size_t elem_size, size_t num_elems) {
    void *ptr;
    Aqueue *queues;

    if(dest == NULL || memory == NULL || elem_size == 0 || num_elems == 0) {
        return 1;
    }

    *dest = memory;
    queues = memory;

    ptr = pointer_literal_addition(queues, num_queues * sizeof(Aqueue));
    for(size_t i = 0; i < num_queues; i++){
        queues[i].elems = (void *) ptr;
        queues[i].elem_size = elem_size;
        queues[i].front = 0;
        queues[i].back = 0;
        __atomic_store_n(&(queues[i].len), 0, __ATOMIC_SEQ_CST);
        queues[i].cap = num_elems;
        ptr = pointer_literal_addition(ptr, aqueue_advise(elem_size, num_elems) - sizeof(Aqueue));
    }
    return 0;
}

void aqueue_deinit(Aqueue *queue) {
    if(queue == NULL || queue->elems == NULL) {
        return;
    }

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
    if(__atomic_load_n(&(queue->len), __ATOMIC_SEQ_CST) == queue->cap) {
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

    if(__atomic_load_n(&(queue->len), __ATOMIC_SEQ_CST) == 0) {
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

    if(__atomic_load_n(&(queue->len), __ATOMIC_SEQ_CST) == 0) {
        return NULL;
    }

    return pointer_literal_addition(queue->elems, queue->elem_size * aqueue_wrap(queue, queue->front));
}

size_t aqueue_len(Aqueue *queue) {
    if(queue == NULL) {
        return 0;
    }

    return __atomic_load_n(&(queue->len), __ATOMIC_SEQ_CST);
}

size_t aqueue_cap(Aqueue *queue) {
    if(queue == NULL) {
        return 0;
    }

    return queue->cap;
}
