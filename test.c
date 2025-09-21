#include "assert.h"
#include "pool.h"
#include <stdlib.h>

Vpool_functions long_functions;
Vpool *longs;

int init_long(void *ptr){
    if(ptr == NULL){
        return 1;
    }

    long *long_ptr = ptr;
    *long_ptr = 1;
    return 0;
}

int deinit_long(void *ptr){
    if(ptr == NULL){
        return 1;
    }

    long *long_ptr = ptr;
    *long_ptr = 0;
    return 0;
}

int main(void){
    long_functions.initialize_element = init_long;
    long_functions.deinitialize_element = deinit_long;
    longs = vpool_create(2, sizeof(long), &long_functions);

    long *a = vpool_alloc(&longs);
    long *b = vpool_alloc(&longs);
    long *c = vpool_alloc(&longs);
    assert(a != NULL && b != NULL && c != NULL);
    assert(a != b && b != c && a != c);

    *a = 1;
    *b = 2;
    *c = 3;

    vpool_dealloc(&longs, a);
    vpool_dealloc(&longs, b);
    vpool_dealloc(&longs, c);

    a = vpool_alloc(&longs);
    b = vpool_alloc(&longs);
    c = vpool_alloc(&longs);

    vpool_dealloc(&longs, a);
    vpool_dealloc(&longs, c);

    vpool_destroy(&longs);

    return 0;
}
