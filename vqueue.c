#include "vqueue.h"
#include "vqueue_priv.h"
#include "pointerarith.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

size_t vqueue_wrap(Vqueue *queue, size_t pos){
    if(queue == NULL){
        return 0;
    }

    return pos % queue->cap;
}

Vqueue *vqueue_init(size_t elem_size, size_t num_elems){
    Vqueue *ret;

    if(elem_size == 0 || num_elems == 0){
        return NULL;
    }

    ret = calloc(1, sizeof(Vqueue) + (elem_size * num_elems));
    if(ret == NULL){
        return NULL;
    }

    ret->elem_size = elem_size;
    ret->front = 0;
    ret->back = 0;
    ret->cap = num_elems;
    return ret;
}

int vqueue_destroy(Vqueue **queue_ptr){
    Vqueue *queue;
    if(queue_ptr == NULL){
        return 1;
    }

    queue = *queue_ptr;
    if(queue == NULL){
        return 2;
    }

    // pop while not empty
    while(vqueue_front(&queue) != NULL){
        if(vqueue_dequeue(&queue) == NULL){
            return 3;
        }
    }

    free(queue);
    return 0;
}

int vqueue_enqueue(Vqueue **queue_ptr, void *elem, bool overwrite){
    Vqueue *queue;
    void *enqueued;
    if(queue_ptr == NULL || elem == NULL){
        return 1;
    }

    queue = *queue_ptr;
    if(queue == NULL){
        return 2;
    }
    
    // check if full
    if(!overwrite && !(queue->front == queue->back && queue->front == 0 && queue->back == 0) && vqueue_wrap(queue, queue->front) == vqueue_wrap(queue, queue->back)){
        return 3;
    }

    enqueued = pointer_literal_addition(queue, sizeof(Vqueue) + (queue->elem_size * vqueue_wrap(queue, queue->back)));
    if(memcpy(enqueued, elem, queue->elem_size) != enqueued){
        return 4;
    }
    queue->back++;
    if(queue->back > queue->cap){
       queue->front++;
    }

    if(queue->front > vqueue_wrap(queue, queue->front) && queue->back > vqueue_wrap(queue, queue->back)){
        queue->front = vqueue_wrap(queue, queue->front);
        queue->back = vqueue_wrap(queue, queue->back);
    }

    return 0;
}

void *vqueue_dequeue(Vqueue **queue_ptr){
    Vqueue *queue;
    void *dequeued;
    if(queue_ptr == NULL){
        return NULL;
    }

    queue = *queue_ptr;
    if(queue == NULL){
        return NULL;
    }

    dequeued = vqueue_front(queue_ptr);
    if(dequeued == NULL){
        return NULL;
    }
    queue->front++;

    if(queue->front > vqueue_wrap(queue, queue->front) && queue->back > vqueue_wrap(queue, queue->back)){
        queue->front = vqueue_wrap(queue, queue->front);
        queue->back = vqueue_wrap(queue, queue->back);
    }

    if(queue->front == queue->back){
        queue->front = 0;
        queue->back = 0;
    }

    return dequeued;
}

void *vqueue_front(Vqueue **queue_ptr){
    Vqueue *queue;
    void *front;
    if(queue_ptr == NULL){
        return NULL;
    }

    queue = *queue_ptr;
    if(queue == NULL){
        return NULL;
    }

    //if(vqueue_wrap(queue, queue->front) == vqueue_wrap(queue, queue->back)){
    if(queue->back == 0){
        return NULL;
    }

    front = pointer_literal_addition(queue, sizeof(Vqueue) + (queue->elem_size * vqueue_wrap(queue, queue->front)));
    if(front == NULL){
        return NULL;
    }
    return front;
}

void *vqueue_back(Vqueue **queue_ptr){
    Vqueue *queue;
    void *back;
    if(queue_ptr == NULL){
        return NULL;
    }

    queue = *queue_ptr;
    if(queue == NULL){
        return NULL;
    }

    //if(vqueue_wrap(queue, queue->front) == vqueue_wrap(queue, queue->back)){
    if(queue->back == 0){
        return NULL;
    }

    back = pointer_literal_addition(queue, sizeof(Vqueue) + (queue->elem_size * vqueue_wrap(queue, queue->back - 1)));
    if(back == NULL){
        return NULL;
    }
    return back;
}
