/*
 * vqueue.h -- Queue containing elements of an arbitrary type
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef struct vqueue {
    // elements that are in the queue placed here
    void *elems;

    // size of individual elements
    size_t elem_size;

    // current position of the first element of the queue
    size_t front;

    // current position of the last element of the queue
    size_t back;

    // need to have distinct length field because back may be less than front
    size_t len;

    // total number of elements this queue can store, if empty
    size_t cap;
} Vqueue;

// Allocates memory for and initializes a Vqueue.
Vqueue *vqueue_create(size_t elem_size, size_t num_elems);

// Advise how much memory is needed for one vqueue with num_elems elements each of elem_size bytes
size_t vqueue_advise(size_t elem_size, size_t num_elems);

// Advise for many
size_t vqueue_advisev(size_t num_queues, size_t elem_size, size_t num_elems);

// Initializes a Vqueue.
int vqueue_init(Vqueue **queue, void *memory, size_t elem_size, size_t num_elems);

// Initialize for many
int vqueue_initv(size_t num_queues, Vqueue *dest[], void *memory, size_t elem_size, size_t num_elems);

// Deinitializes a Vqueue>
void vqueue_deinit(Vqueue *queue);

/*
 * Destroys a Vqueue that was allocated by vqueue_create.
 * Please, only use with memory allocated by vqueue_create!
 */
void vqueue_destroy(Vqueue *queue);

// Enqueues contents of src into queue.
int vqueue_enqueue(Vqueue *queue, void *src, bool overwrite);

// Dequeues front element of queue into dest.
int vqueue_dequeue(Vqueue *queue, void *dest);

// Try to enqueue several elements
// The amount that were able to be enqueued is stored in num_enqueued
int vqueue_enqueue_some(Vqueue *queue, size_t *num_enqueued, size_t enqueue_this_many, void *src, bool overwrite);

// Try to dequeue several elements
// The amount that were able to be dequeued is stored in num_dequeued
int vqueue_dequeue_some(Vqueue *queue, size_t *num_dequeued, size_t dequeue_this_many, void *dest);

// Gets the contents of the front of the queue.
int vqueue_front(Vqueue *queue, void *dest);

/*
 * Gets the memory address of the element in the front of the queue.
 * Should be used carefully as further enqeue/dequeue operations can overwrite this memory.
 * Thus, storing the pointer returned from this across operations almost always introduces a bug!
 */
void *vqueue_front_direct(Vqueue *queue);

// Gets the contents of the back of the queue.
int vqueue_back(Vqueue *queue, void *dest);

/*
 * Gets the memory address of the element in the back of the queue.
 * Should be used carefully as further enqeue/dequeue operations can overwrite this memory.
 * Thus, storing the pointer returned from this across operations almost always introduces a bug!
 */
void *vqueue_back_direct(Vqueue *queue);

/*
 * Returns current length of the queue.
 * Calculated as number of elements currently enqueued.
 * Decreases when elements are dequeued.
 */
size_t vqueue_len(Vqueue *queue);

/*
 * Returns the total number of elements that can be enqueued without overwriting anything if the queue is empty.
 * Calculated as number of elements the queue was told to allocate when creating/initializing.
 */
size_t vqueue_cap(Vqueue *queue);
