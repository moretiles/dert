#include <vqueue.h>
#include <vqueue_priv.h>
#include <pointerarith.h>

#include <errno.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

size_t vqueue_wrap(Vqueue *queue, size_t pos) {
    if(queue == NULL) {
        return 0;
    }

    return pos % queue->cap;
}

Vqueue *vqueue_create(size_t elem_size, size_t num_elems) {
    void *memory;
    Vqueue *ret;

    if(elem_size == 0 || num_elems == 0) {
        return NULL;
    }

    memory = malloc(vqueue_advise(elem_size, num_elems));
    if(memory == NULL) {
        return NULL;
    }

    if(vqueue_init(&ret, memory, elem_size, num_elems) != 0) {
        free(memory);
        return NULL;
    }
    return ret;
}

size_t vqueue_advise(size_t elem_size, size_t num_elems) {
    return (1 * sizeof(Vqueue)) +
           (num_elems * elem_size);
}

size_t vqueue_advisev(size_t num_queues, size_t elem_size, size_t num_elems) {
    return num_queues * ((1 * sizeof(Vqueue)) +
                         (num_elems * elem_size));
}

int vqueue_init(Vqueue **dest, void *memory, size_t elem_size, size_t num_elems) {
    Vqueue *queue;
    void *ptr;

    if(dest == NULL || memory == NULL || elem_size == 0 || num_elems == 0) {
        return EINVAL;
    }

    if(memset(memory, 0, vqueue_advise(elem_size, num_elems)) != memory) {
        return ENOTRECOVERABLE;
    }
    ptr = memory;
    ptr = pointer_literal_addition(ptr, 0);
    queue = (Vqueue *) ptr;
    ptr = pointer_literal_addition(ptr, 1 * sizeof(Vqueue));
    queue->elems = (void *) ptr;

    queue->elem_size = elem_size;
    queue->front = 0;
    queue->back = 0;
    queue->cap = num_elems;
    queue->len = 0;

    *dest = queue;
    return 0;
}

int vqueue_initv(size_t num_queues, Vqueue *dest[], void *memory, size_t elem_size, size_t num_elems) {
    void *ptr;
    Vqueue *queues;

    if(dest == NULL || memory == NULL || elem_size == 0 || num_elems == 0) {
        return EINVAL;
    }

    *dest = memory;
    queues = memory;

    if(memset(queues, 0, vqueue_advisev(num_queues, elem_size, num_elems)) != queues) {
        return ENOTRECOVERABLE;
    }

    ptr = pointer_literal_addition(queues, num_queues * sizeof(Vqueue));
    for(size_t i = 0; i < num_queues; i++) {
        queues[i].elems = (void *) ptr;
        queues[i].elem_size = elem_size;
        queues[i].front = 0;
        queues[i].back = 0;
        queues[i].cap = num_elems;
        queues[i].len = 0;
        ptr = pointer_literal_addition(ptr, vqueue_advise(elem_size, num_elems) - sizeof(Vqueue));
    }
    return 0;
}

void vqueue_deinit(Vqueue *queue) {
    if(queue == NULL || queue->elems == NULL) {
        return;
    }

    queue->len = 0;
    memset(queue, 0, sizeof(Vqueue));
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
    bool something_will_be_overwritten = false;

    if(queue == NULL || src == NULL) {
        return EINVAL;
    }

    if(queue->len == queue->cap) {
        if(overwrite) {
            something_will_be_overwritten = true;
        } else {
            // fail if full
            return EXFULL;
        }
    }

    enqueued = pointer_literal_addition(queue->elems, queue->elem_size * vqueue_wrap(queue, queue->back));
    if(memcpy(enqueued, src, queue->elem_size) != enqueued) {
        return ENOTRECOVERABLE;
    }
    queue->back = vqueue_wrap(queue, queue->back + 1);
    if(something_will_be_overwritten) {
        queue->front = vqueue_wrap(queue, queue->front + 1);
    } else {
        queue->len += 1;
    }

    return 0;
}

int vqueue_dequeue(Vqueue *queue, void *dest) {
    if(queue == NULL) {
        return EINVAL;
    }

    // fails when queue->len == 0
    if(vqueue_front(queue, dest) != 0) {
        return ENODATA;
    }
    queue->front = vqueue_wrap(queue, queue->front + 1);
    queue->len -= 1;

    return 0;
}

int vqueue_enqueue_some(Vqueue *queue, size_t *num_enqueued, size_t enqueue_this_many, void *src, bool overwrite) {
    int res;
    if(queue == NULL || num_enqueued == NULL || enqueue_this_many == 0 || src == NULL) {
        return EINVAL;
    }

    size_t how_many_can_be_enqueued_without_overwriting = vqueue_cap(queue) - vqueue_len(queue);
    size_t how_many_can_be_enqueued_with_overwriting = 0;
    if(overwrite) {
        how_many_can_be_enqueued_with_overwriting = vqueue_len(queue);
    } else {
        how_many_can_be_enqueued_with_overwriting = 0;
    }
    size_t how_many_can_be_enqueued = (
                                          how_many_can_be_enqueued_with_overwriting + how_many_can_be_enqueued_without_overwriting
                                      );
    if(how_many_can_be_enqueued == 0) {
        return EXFULL;
    }
    size_t how_many_should_be_enqueued = 0;
    if(how_many_can_be_enqueued >= enqueue_this_many) {
        how_many_should_be_enqueued = enqueue_this_many;
        res = 0;
    } else {
        how_many_should_be_enqueued = how_many_can_be_enqueued;
        res = EXFULL;
    }

    size_t enqueue_this_many_before_wrapping = queue->cap - vqueue_wrap(queue, queue->back);
    void *dest = pointer_literal_addition(queue->elems, queue->elem_size * vqueue_wrap(queue, queue->back));
    if(enqueue_this_many_before_wrapping >= how_many_should_be_enqueued) {
        memcpy(dest, src, queue->elem_size * how_many_should_be_enqueued);
    } else {
        memcpy(dest, src, queue->elem_size * enqueue_this_many_before_wrapping);
        size_t enqueue_this_many_after_wrapping = how_many_should_be_enqueued - enqueue_this_many_before_wrapping;
        src = pointer_literal_addition(src, queue->elem_size * enqueue_this_many_before_wrapping);
        memcpy(queue->elems, src, queue->elem_size * enqueue_this_many_after_wrapping);
    }

    queue->back += how_many_should_be_enqueued;
    if(how_many_should_be_enqueued > how_many_can_be_enqueued_without_overwriting) {
        queue->front += (how_many_should_be_enqueued - how_many_can_be_enqueued_without_overwriting);
        queue->len += how_many_can_be_enqueued_without_overwriting;
    } else {
        queue->len += how_many_should_be_enqueued;
    }
    *num_enqueued = how_many_should_be_enqueued;
    return res;
}

int vqueue_dequeue_some(Vqueue *queue, size_t *num_dequeued, size_t dequeue_this_many, void *dest) {
    int res;
    if(queue == NULL || num_dequeued == NULL || dequeue_this_many == 0 || dest == NULL) {
        return EINVAL;
    }

    size_t how_many_can_be_dequeued = vqueue_len(queue);
    if(how_many_can_be_dequeued == 0) {
        return ENODATA;
    }
    size_t how_many_should_be_dequeued;
    if(how_many_can_be_dequeued >= dequeue_this_many) {
        how_many_should_be_dequeued = dequeue_this_many;
        res = 0;
    } else {
        how_many_should_be_dequeued = how_many_can_be_dequeued;
        res = ENODATA;
    }

    size_t dequeue_this_many_before_wrapping = queue->cap - vqueue_wrap(queue, queue->front);
    void *src = pointer_literal_addition(queue->elems, queue->elem_size * vqueue_wrap(queue, queue->front));
    if(dequeue_this_many_before_wrapping >= how_many_should_be_dequeued) {
        memcpy(dest, src, queue->elem_size * how_many_should_be_dequeued);
    } else {
        memcpy(dest, src, queue->elem_size * dequeue_this_many_before_wrapping);
        size_t dequeue_this_many_after_wrapping = how_many_should_be_dequeued - dequeue_this_many_before_wrapping;
        dest = pointer_literal_addition(dest, queue->elem_size * dequeue_this_many_before_wrapping);
        memcpy(dest, queue->elems, queue->elem_size * dequeue_this_many_after_wrapping);
    }

    queue->front += how_many_should_be_dequeued;
    queue->len -= how_many_should_be_dequeued;
    *num_dequeued = how_many_should_be_dequeued;
    return res;
}

int vqueue_front(Vqueue *queue, void *dest) {
    void *front;

    if(queue == NULL) {
        return EINVAL;
    }

    front = vqueue_front_direct(queue);
    if(front == NULL) {
        // the single producer, single consumer use case expected has been broken
        return ENOTRECOVERABLE;
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

    // fails when queue->len == 0
    if(queue->len == 0) {
        return NULL;
    }

    return pointer_literal_addition(queue->elems, queue->elem_size * vqueue_wrap(queue, queue->front));
}

int vqueue_back(Vqueue *queue, void *dest) {
    void *back;

    if(queue == NULL) {
        return EINVAL;
    }

    back = vqueue_back_direct(queue);
    if(back == NULL) {
        // the single producer, single consumer use case expected has been broken
        return ENOTRECOVERABLE;
    }
    if(memcpy(dest, back, queue->elem_size) != dest) {
        return ENOTRECOVERABLE;
    }
    return 0;
}

void *vqueue_back_direct(Vqueue *queue) {
    size_t index_of_back_element;
    if(queue == NULL) {
        return NULL;
    }

    // fails when queue->len == 0
    if(queue->len == 0) {
        return NULL;
    }

    // back is always incremented by 1 ahead of time to make it easy to enqueue the next element
    // need to decrement and wrap around to get the real index of the back element
    index_of_back_element = vqueue_wrap(queue, queue->cap + queue->back - 1);
    return pointer_literal_addition(queue->elems, queue->elem_size * index_of_back_element);
}

size_t vqueue_len(Vqueue *queue) {
    if(queue == NULL) {
        return 0;
    }

    return queue->len;
}

size_t vqueue_cap(Vqueue *queue) {
    if(queue == NULL) {
        return 0;
    }

    return queue->cap;
}
