#include "fqueue.h"

#include <stdlib.h>

Fqueue *fqueue_create(size_t num_bytes, const char *file_name, const char *mode){
    Fqueue *ret;
    FILE *file;
    
    if(num_bytes == 0 || file_name == NULL || mode == NULL){
        return NULL;
    }

    ret = calloc(1, sizeof(Fqueue));
    if(ret == NULL){
        return NULL;
    }

    file = fopen(file_name, mode);
    if(file == NULL){
        free(ret);
        return NULL;
    }

    if(fqueue_init(ret, num_bytes, file) != 0){
        free(ret);
        fclose(file);
        return NULL;
    }

    return ret;
}

int fqueue_init(Fqueue *queue, size_t num_bytes, FILE *file){
    char *bytes;

    if(queue == NULL || num_bytes == 0 || file == NULL){
        return 1;
    }

    bytes = calloc(sizeof(char), num_bytes);
    if(bytes == NULL){
        return 2;
    }

    queue->file = file;
    queue->bytes = bytes;
    queue->writeCursor = 0;
    queue->readCursor = 0;
    queue->cap = num_bytes;
    return 0;
}

void fqueue_deinit(Fqueue *queue){
    if(queue == NULL){
        return;
    }

    if(queue->file != NULL){
        fclose(queue->file);
        queue->file = NULL;
    }

    if(queue->bytes != NULL){
        free(queue->bytes);
        queue->bytes = NULL;
    }

    memset(queue, 0, sizeof(Fqueue));
    return;
}

void fqueue_destroy(Fqueue *queue){
    if(queue == NULL){
        return;
    }

    fqueue_deinit(queue);
    free(queue);
    return;
}

int fqueue_len(Fqueue *queue){
    if(queue == NULL){
        return 0;
    }

    return queue->writeCursor - queue->readCursor;
}

int fqueue_dequeue(Fqueue *store, char *data, int size) {
    if (size <= 0) {
        return ERR_QUEUE_INVALID_SIZE;
    }
    if (size > store->writeCursor - store->readCursor) {
        size = store->writeCursor - store->readCursor;
    }

    memcpy(data, store->bytes + store->readCursor, size);
    store->readCursor = store->readCursor + size;
    return size;
}

int fqueue_enqueue(Fqueue *store, char *data, int size) {
    if (size <= 0) {
        return ERR_QUEUE_INVALID_SIZE;
    }
    if (store->writeCursor + size > store->cap) {
        return ERR_QUEUE_OUT_OF_MEMORY;
    }

    memcpy(store->bytes + store->writeCursor, data, size);
    store->writeCursor = store->writeCursor + size;
    return size;
}

/*
 * Move size bytes from readQueue to writeQueue
 */
int fqueue_exchange(Fqueue *readQueue, Fqueue *writeQueue, int size) {
    if (size <= 0) {
        return ERR_QUEUE_INVALID_SIZE;
    }
    if (size > readQueue->writeCursor - readQueue->readCursor) {
        size = readQueue->writeCursor - readQueue->readCursor;
    }
    if (writeQueue->writeCursor + size > writeQueue->cap) {
        return ERR_QUEUE_OUT_OF_MEMORY;
    }
    memcpy(writeQueue->bytes + writeQueue->writeCursor,
           readQueue->bytes + readQueue->readCursor,
           size);
    readQueue->readCursor = readQueue->readCursor + size;
    writeQueue->writeCursor = writeQueue->writeCursor + size;
    return size;
}

// Dequeue a single byte from store->bytes
int fqueue_dequeuec(Fqueue *store, char *cptr) {
    return fqueue_dequeue(store, cptr, 1);
}

// Enqueue a single byte to store->bytes
int fqueue_enqueuec(Fqueue *store, char c) {
    return fqueue_enqueue(store, &c, 1);
}

// Copy from store->bytes over [readCursor, writeCursor) to start of store->bytes
int fqueue_fold_down(Fqueue *store) {
    int diff = store->writeCursor - store->readCursor;
    if (diff > store->readCursor) {
        return ERR_QUEUE_OUT_OF_MEMORY;
    }

    if(diff > 0){
        memmove(store->bytes, store->bytes + store->readCursor, diff);
    }
    store->writeCursor = diff;
    store->readCursor = 0;
    return diff;
}

// Enqueue MAX_BLOCK_SIZE bytes from attached file into store->bytes
int fqueue_fenqueue(Fqueue *store, int size) {
    int read = 0;
    if (store->writeCursor + size > store->cap) {
        return ERR_QUEUE_OUT_OF_MEMORY;
    }
    if (ferror(store->file)) {
        return ERR_QUEUE_FILE_IO;
    }

    read = fread(store->bytes + store->writeCursor, 1, size, store->file);
    store->writeCursor = store->writeCursor + read;
    return read;
}

// Dequeue MAX_BLOCK_SIZE bytes in store->bytes to a file
int fqueue_fdequeue(Fqueue *store, int size) {
    int difference = 0;
    int write = 0;
    if (store->writeCursor == 0) {
        return ERR_QUEUE_EMPTY;
    }
    if (ferror(store->file)) {
        return ERR_QUEUE_FILE_IO;
    }

    difference = store->writeCursor - store->readCursor;
    size = (size > difference) ? difference : size;
    write = fwrite(store->bytes, 1, size, store->file);
    store->writeCursor = store->writeCursor - difference;
    if (store->writeCursor < 0) {
        store->writeCursor = 0;
    }
    return write;
}

int fqueue_rewind_read_cursor(Fqueue *store, int back) {
    if (store->readCursor - back < 0) {
        return ERR_QUEUE_INVALID_SIZE;
    }

    store->readCursor -= back;
    return back;
}

int fqueue_rewind_write_cursor(Fqueue *store, int back) {
    if (store->writeCursor - back < 0) {
        return ERR_QUEUE_INVALID_SIZE;
    }

    store->writeCursor -= back;
    if(store->writeCursor < store->readCursor){
        store->readCursor = store->writeCursor;
    }
    return back;
}

/*
bool fqueue_queueCanHold(Fqueue *queue, int size) {
    if(queue == NULL) {
        return false;
    }

    return queue->writeCursor + size < queue->cap;
}

int fqueue_queueEnsureSpace(Fqueue *queue, int max) {
    if(queue == NULL) {
        return 1;
    }

    if(!queueCanHold(queue, max)) {
        fdequeue(queue, queue->writeCursor - queue->readCursor);
        queue->writeCursor = 0;
        queue->readCursor = 0;
    }

    return 0;
}
*/
