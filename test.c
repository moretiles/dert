#include "assert.h"
#include "pool.h"

Vpool *longs;

int main(void){
    longs = vpool_create(64, sizeof(long), NULL, NULL);

    long *a = vpool_alloc(&longs);
    long *b = vpool_alloc(&longs);
    long *c = vpool_alloc(&longs);
    assert(a != NULL && b != NULL && c != NULL);
    assert(a != b && b != c && a != c);

    *a = 1;
    *b = 2;
    *c = 3;

    vpool_dealloc(&longs, (void**) a);
    vpool_dealloc(&longs, (void**) b);
    vpool_dealloc(&longs, (void**) c);

    a = vpool_alloc(&longs);
    b = vpool_alloc(&longs);
    c = vpool_alloc(&longs);

    vpool_destroy(&longs);

    return 0;
}
