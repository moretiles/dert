#include "assert.h"
#include "vpool.h"
#include "vdll.h"
#include "varray.h"

#include <stdlib.h>
#include <time.h>

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

int vpool_test(void){
    Vpool_functions long_functions;
    Vpool *longs;

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

int vdll_test(){
    srand(time(NULL));

    Vdll_functions functions = { .init = init_long, .deinit = deinit_long };
    #define TEST_VDLL_ARRAY_LEN (99)
    Vdll *dll = vdll_init(sizeof(long), &functions);
    assert(dll != NULL);
    assert(vdll_grow(dll, TEST_VDLL_ARRAY_LEN) == 0);

    long array[TEST_VDLL_ARRAY_LEN];
    long tmp;
    for(size_t i = 0; i < TEST_VDLL_ARRAY_LEN * 10; i++){
        size_t pos = rand() % TEST_VDLL_ARRAY_LEN;
        array[pos] = rand();
        assert(vdll_set(dll, pos, &(array[pos])) == 0);
    }

    for(size_t i = 0; i < TEST_VDLL_ARRAY_LEN * 100; i++){
        size_t pos = rand() % TEST_VDLL_ARRAY_LEN;
        assert(vdll_get(dll, pos, &tmp) == 0);
        assert(tmp == array[pos]);
    }

    assert(vdll_len(dll) == TEST_VDLL_ARRAY_LEN);
    assert(vdll_shrink(dll, 1 + (TEST_VDLL_ARRAY_LEN / 2)) == 0);
    assert(vdll_destroy(dll) == 0);

    return 0;
}

int varray_test(){
    srand(time(NULL));

    Varray *array = varray_init(sizeof(long));
    assert(array != NULL);
    assert(varray_grow(&array, 4) == 0);
    assert(varray_grow(&array, 12) == 0);

    long a = 1, b = 2, c = 3;
    assert(varray_set(&array, 0, &a) == 0);
    assert(varray_set(&array, 1, &b) == 0);
    assert(varray_set(&array, 2, &c) == 0);

    long *a_ptr, *b_ptr, *c_ptr;
    a_ptr = varray_get(&array, 0);
    b_ptr = varray_get(&array, 1);
    c_ptr = varray_get(&array, 2);

    assert(a_ptr != NULL);
    assert(b_ptr != NULL);
    assert(c_ptr != NULL);

    assert(*a_ptr == a);
    assert(*b_ptr == b);
    assert(*c_ptr == c);

    assert(varray_shrink(&array, 8) == 0);
    assert(varray_destroy(&array) == 0);

    return 0;
}

int main(){
    vpool_test();
    vdll_test();
    varray_test();
}
