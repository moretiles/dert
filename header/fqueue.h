/*
 * fqueue.h -- Queue designed for simple file I/O
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define ERR_QUEUE_INVALID_SIZE (1)
#define ERR_QUEUE_NULL (2)
#define ERR_QUEUE_CANNOT_READ_QUANTITY (3)
#define ERR_QUEUE_CANNOT_WRITE_QUANTITY (4)
#define ERR_QUEUE_FILE_OPEN (5)
#define ERR_QUEUE_FILE_IO (6)
#define ERR_QUEUE_FILE_READ_INCOMPLETE (7)
#define ERR_QUEUE_FILE_WRITE_INCOMPLETE (8)

#ifndef FQUEUE_STRUCT
#define FQUEUE_STRUCT 1
typedef struct fqueue {
    //  File associated with queue
    FILE *file;

    // Bytes used for reading and writing
    char *bytes;

    // Offset into bytes to begin next write at
    size_t writeCursor;

    // Offset into bytes to begin next read at
    size_t readCursor;

    // Total number of bytes that can be stored in fqueue->bytes
    size_t cap;
} Fqueue;
#endif

// Allocates memory for and initializes a Fqueue.
Fqueue *fqueue_create(size_t num_bytes, const char *file_name, const char *mode);

// Initializes a Fqueue.
int fqueue_init(Fqueue *queue, size_t num_bytes, FILE *file);

// Deinitializes a Fqueue.
void fqueue_deinit(Fqueue *queue);

/*
 * Destroys a Fqueue that was allocated by fqueue_create.
 * Please, only use with memory allocated by fqueue_create!
 */
void fqueue_destroy(Fqueue *queue);

// Get number of previously read bytes still loaded in queue
size_t fqueue_prev(Fqueue *queue);

// Get used space of queue
size_t fqueue_used(Fqueue *queue);

// Get unused space of queue
size_t fqueue_unused(Fqueue *queue);

// Get length of queue
size_t fqueue_cap(Fqueue *queue);

// Enqueue size bytes to store->bytes
int fqueue_enqueue(Fqueue *store, char *data, size_t size);

// Enqueue a single byte to store->bytes
int fqueue_enqueuec(Fqueue *store, char c);

/*
 * Dequeue size bytes from store->bytes to data
 * Uses readCursor to know what the bottom should be
 */
int fqueue_dequeue(Fqueue *store, char *data, size_t size);

// Dequeue a single byte from store->bytes
int fqueue_dequeuec(Fqueue *store, char *cptr);

// Enqueue size bytes from attached file into store->bytes
int fqueue_fenqueue(Fqueue *store, size_t size);

// Dequeue size bytes in store->bytes to a file
int fqueue_fdequeue(Fqueue *store, size_t size);

/* 
 * Decrement the read cursor of store by len
 * Useful to read bytes you've already read from store
 */
int fqueue_rewind_read_cursor(Fqueue *store, size_t len);

/*
 * Decrement the write cursor of store by len
 * Useful to (prepare) to overwrite bytes written to store
 */
int fqueue_rewind_write_cursor(Fqueue *store, size_t len);

/*
 * Move size bytes from readQueue to writeQueue
 */
int fqueue_exchange(Fqueue *readQueue, Fqueue *writeQueue, size_t size);

// Copy from store->bytes over [readCursor, writeCursor) to start of store->bytes
int fqueue_fold_down(Fqueue *store);
