/*
 * aqueue.h -- Atomic queue designed for single producer, single consumer workflow
 * This structure is thread safe to use when one producer thread enqueues and one consumer thread dequeues
 * This structure does not overwrite elements that are currently queued unless they have been dequeued
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef struct aqueue {
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
} Aqueue;

// Allocates memory for and initializes a Aqueue.
Aqueue *aqueue_create(size_t elem_size, size_t num_elems);

// Advise how much memory an Aqueue with a capacity of num_elems needs
// Assumes that the same value for num_elems is used when calling tbuf_init
size_t aqueue_advise(size_t elem_size, size_t num_elems);

// Advise for many
size_t aqueue_advisev(size_t num_queues, size_t elem_size, size_t num_elems);

// Initializes a Aqueue
int aqueue_init(Aqueue **dest, void *memory, size_t elem_size, size_t num_elems);

// Initialize for many
int aqueue_initv(size_t num_queues, Aqueue *dest[], void *memory, size_t elem_size, size_t num_elems);

// Deinitializes a Aqueue
void aqueue_deinit(Aqueue *queue);

/*
 * Destroys a Aqueue that was allocated by aqueue_create.
 * Please, only use with memory allocated by aqueue_create!
 */
void aqueue_destroy(Aqueue *queue);

// Enqueues contents of src into queue.
int aqueue_enqueue(Aqueue *queue, void *src);

// Dequeues front element of queue into dest.
int aqueue_dequeue(Aqueue *queue, void *dest);

// Gets the contents of the front of the queue.
int aqueue_front(Aqueue *queue, void *dest);

/*
 * Gets the memory address of the element in the front of the queue.
 * Should be used carefully as further enqeue/dequeue operations can overwrite this memory.
 * Thus, storing the pointer returned from this across operations almost always introduces a bug!
 */
void *aqueue_front_direct(Aqueue *queue);

/*
 * Returns current length of the queue.
 * Calculated as number of elements currently enqueued.
 * Decreases when elements are dequeued.
 */
size_t aqueue_len(Aqueue *queue);

/*
 * Returns the total number of elements that can be enqueued without overwriting anything if the queue is empty.
 * Calculated as number of elements the queue was told to allocate when creating/initializing.
 */
size_t aqueue_cap(Aqueue *queue);
