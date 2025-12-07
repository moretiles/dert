/*
 * mpscqueue_priv.h -- Multi-producer, single-consumer queue
 * This structure is thread safe to use when multiple producer threads enqueue and one consumer thread dequeues
 * This structure does not overwrite elements that are currently queued unless they have been dequeued
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>

typedef struct mpscqueue {
    // elements that are in the queue placed here
    void *elems;

    // size of individual elements
    size_t elem_size;

    // current position of the first element of the queue
    size_t front;

    // current position of the last element of the queue
    size_t back;

    // need to have distinct length field because back may be less than front
    _Atomic size_t len;

    // total number of elements this queue can store, if empty
    size_t cap;

    // mutex used to synchronize the multiple producers
    pthread_mutex_t producer_mutex;
} Mpscqueue;

// Allocates memory for and initializes a Mpscqueue.
Mpscqueue *mpscqueue_create(size_t elem_size, size_t num_elems);

// Initializes a Mpscqueue.
int mpscqueue_init(Mpscqueue *queue, size_t elem_size, size_t num_elems);

// Deinitializes a Mpscqueue>
void mpscqueue_deinit(Mpscqueue *queue);

/*
 * Destroys a Mpscqueue that was allocated by mpscqueue_create.
 * Please, only use with memory allocated by mpscqueue_create!
 */
void mpscqueue_destroy(Mpscqueue *queue);

// Enqueues contents of src into queue.
int mpscqueue_enqueue(Mpscqueue *queue, void *src);

// Dequeues front element of queue into dest.
int mpscqueue_dequeue(Mpscqueue *queue, void *dest);

// Gets the contents of the front of the queue.
int mpscqueue_front(Mpscqueue *queue, void *dest);

/*
 * Gets the memory address of the element in the front of the queue.
 * Should be used carefully as further enqeue/dequeue operations can overwrite this memory.
 * Thus, storing the pointer returned from this across operations almost always introduces a bug!
 */
void *mpscqueue_front_direct(Mpscqueue *queue);

/*
 * Returns current length of the queue.
 * Calculated as number of elements currently enqueued.
 * Decreases when elements are dequeued.
 */
size_t mpscqueue_len(Mpscqueue *queue);

/*
 * Returns the total number of elements that can be enqueued without overwriting anything if the queue is empty.
 * Calculated as number of elements the queue was told to allocate when creating/initializing.
 */
size_t mpscqueue_cap(Mpscqueue *queue);
