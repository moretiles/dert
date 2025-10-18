/*
 * queue.h -- header file that provides queue data structure to represent file
 *
 * dank_json - Another tool for working with JSON in C
 * https://github.com/moretiles/dank_json
 * Project licensed under Apache-2.0 license
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Make sure that MAX_BLOCK_SIZE is a multiple of 3 and 4
#ifndef MAX_BLOCK_SIZE
#define MAX_BLOCK_SIZE (3 * 1024 * 1024)
#endif

#define ERR_QUEUE_OUT_OF_MEMORY (-1 * (1 << 0))
#define ERR_QUEUE_INVALID_SIZE (-1 * (1 << 1))
#define ERR_QUEUE_EMPTY (-1 * (1 << 2))
#define ERR_QUEUE_FILE_OPEN (-1 * (1 << 8))
#define ERR_QUEUE_FILE_IO (-1 * (1 << 9))

#ifndef FQUEUE_STRUCT
#define FQUEUE_STRUCT 1
typedef struct fqueue {
    //  File associated with queue
    FILE *file;

    // Bytes used for reading and writing
    char *bytes;

    // Offset into bytes to begin next write at
    int writeCursor;

    // Offset into bytes to begin next read at
    int readCursor;

    // Total number of bytes that can be stored in fqueue->bytes
    int cap;
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

// Get length of queue
int fqueue_len(Fqueue *queue);

// Enqueue size bytes to store->bytes
int fqueue_enqueue(Fqueue *store, char *data, int size);

// Enqueue a single byte to store->bytes
int fqueue_enqueuec(Fqueue *store, char c);

/*
 * Dequeue size bytes from store->bytes to data
 * Uses readCursor to know what the bottom should be
 */
int fqueue_dequeue(Fqueue *store, char *data, int size);

// Dequeue a single byte from store->bytes
int fqueue_dequeuec(Fqueue *store, char *cptr);

// Enqueue MAX_BLOCK_SIZE bytes from attached file into store->bytes
int fqueue_fenqueue(Fqueue *store, int size);

// Dequeue MAX_BLOCK_SIZE bytes in store->bytes to a file
int fqueue_fdequeue(Fqueue *store, int size);

/* 
 * Decrement the read cursor of store by len
 * Useful to read bytes you've already read from store
 */
int fqueue_rewind_read_cursor(Fqueue *store, int len);

/*
 * Decrement the write cursor of store by len
 * Useful to (prepare) to overwrite bytes written to store
 */
int fqueue_rewind_write_cursor(Fqueue *store, int len);

/*
 * Move size bytes from readQueue to writeQueue
 */
int fqueue_exchange(Fqueue *readQueue, Fqueue *writeQueue, int size);

// Copy from store->bytes over [readCursor, writeCursor) to start of store->bytes
int fqueue_fold_down(Fqueue *store);

/*
bool fqueue_queueCanHold(Fqueue *queue, int size);

int fqueue_queueEnsureSpace(Fqueue *store, int max);
*/
