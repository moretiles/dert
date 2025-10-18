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

size_t fqueue_prev(Fqueue *queue){
    if(queue == NULL){
        return 0;
    }

    return queue->readCursor;
}

size_t fqueue_used(Fqueue *queue){
    if(queue == NULL){
        return 0;
    }

    return queue->writeCursor - queue->readCursor;
}

size_t fqueue_unused(Fqueue *queue){
    if(queue == NULL){
        return 0;
    }

    return queue->cap - queue->writeCursor;
}

size_t fqueue_cap(Fqueue *queue){
    if(queue == NULL){
        return 0;
    }

    return queue->cap;
}

int fqueue_dequeue(Fqueue *store, char *data, size_t size) {
    if(store == NULL || data == NULL){
        return ERR_QUEUE_NULL;
    }
    if (size == 0) {
        return ERR_QUEUE_INVALID_SIZE;
    }
    if (size > fqueue_used(store)) {
        return ERR_QUEUE_CANNOT_WRITE_QUANTITY;
    }

    memcpy(data, store->bytes + store->readCursor, size);
    store->readCursor = store->readCursor + size;
    return 0;
}

int fqueue_enqueue(Fqueue *store, char *data, size_t size) {
    if(store == NULL || data == NULL){
        return ERR_QUEUE_NULL;
    }
    if (size == 0) {
        return ERR_QUEUE_INVALID_SIZE;
    }
    if (size > fqueue_unused(store)) {
        return ERR_QUEUE_CANNOT_READ_QUANTITY;
    }

    memcpy(store->bytes + store->writeCursor, data, size);
    store->writeCursor = store->writeCursor + size;
    return 0;
}

int fqueue_exchange(Fqueue *readQueue, Fqueue *writeQueue, size_t size) {
    if(readQueue == NULL || writeQueue == NULL){
        return ERR_QUEUE_NULL;
    }
    if (size == 0) {
        return ERR_QUEUE_INVALID_SIZE;
    }
    if (size > fqueue_used(readQueue)) {
        return ERR_QUEUE_CANNOT_READ_QUANTITY;
    }
    if(size > fqueue_unused(writeQueue)){
        return ERR_QUEUE_CANNOT_WRITE_QUANTITY;
    }

    memcpy(writeQueue->bytes + writeQueue->writeCursor,
           readQueue->bytes + readQueue->readCursor,
           size);
    readQueue->readCursor = readQueue->readCursor + size;
    writeQueue->writeCursor = writeQueue->writeCursor + size;
    return 0;
}

int fqueue_dequeuec(Fqueue *store, char *cptr) {
    return fqueue_dequeue(store, cptr, 1);
}

int fqueue_enqueuec(Fqueue *store, char c) {
    return fqueue_enqueue(store, &c, 1);
}

int fqueue_fold_down(Fqueue *store) {
    if(store == NULL){
        return ERR_QUEUE_NULL;
    }

    size_t diff = store->writeCursor - store->readCursor;
    if (diff > (1 + (store->cap / 2))) {
        return ERR_QUEUE_CANNOT_WRITE_QUANTITY;
    }

    memmove(store->bytes, store->bytes + store->readCursor, diff);
    store->writeCursor = diff;
    store->readCursor = 0;
    return 0;
}

int fqueue_fenqueue(Fqueue *store, size_t size) {
    size_t read;

    if(store == NULL){
        return ERR_QUEUE_NULL;
    }
    if (size == 0) {
        return ERR_QUEUE_INVALID_SIZE;
    }
    if(size > fqueue_unused(store)){
        return ERR_QUEUE_CANNOT_READ_QUANTITY;
    }
    if (ferror(store->file)) {
        return ERR_QUEUE_FILE_IO;
    }

    read = fread(store->bytes + store->writeCursor, 1, size, store->file);
    if(read != size){
        return ERR_QUEUE_FILE_READ_INCOMPLETE;
    }
    store->writeCursor = store->writeCursor + read;
    return 0;
}

int fqueue_fdequeue(Fqueue *store, size_t size) {
    if(store == NULL){
        return ERR_QUEUE_NULL;
    }
    if (size == 0) {
        return ERR_QUEUE_INVALID_SIZE;
    }

    size_t difference = 0;
    size_t write = 0;
    if(size > fqueue_used(store)){
        return ERR_QUEUE_CANNOT_WRITE_QUANTITY;
    }
    if (ferror(store->file)) {
        return ERR_QUEUE_FILE_IO;
    }

    difference = store->writeCursor - store->readCursor;
    size = (size > difference) ? difference : size;
    write = fwrite(store->bytes, 1, size, store->file);
    if(write != size){
        return ERR_QUEUE_FILE_WRITE_INCOMPLETE;
    }
    store->writeCursor = store->writeCursor - difference;
    return 0;
}

int fqueue_rewind_read_cursor(Fqueue *store, size_t back) {
    if(store == NULL){
        return ERR_QUEUE_NULL;
    }

    if (back == 0) {
        return ERR_QUEUE_INVALID_SIZE;
    }
    if (store->readCursor < back) {
        return ERR_QUEUE_CANNOT_WRITE_QUANTITY;
    }

    store->readCursor -= back;
    return 0;
}

int fqueue_rewind_write_cursor(Fqueue *store, size_t back) {
    if(store == NULL){
        return ERR_QUEUE_NULL;
    }

    if (back == 0) {
        return ERR_QUEUE_INVALID_SIZE;
    }
    if (store->writeCursor < back) {
        return ERR_QUEUE_CANNOT_WRITE_QUANTITY;
    }

    store->writeCursor -= back;
    if(store->writeCursor < store->readCursor){
        store->readCursor = store->writeCursor;
    }
    return 0;
}
