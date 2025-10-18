#include "assert.h"
#include "vpool.h"
#include "vdll.h"
#include "varray.h"
#include "vstack.h"
#include "vqueue.h"
#include "vht.h"
#include "fqueue.h"

#include <stdlib.h>
#include <time.h>
#include <stdio.h>

int seed;

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

int vdll_test(void){
    Vdll_functions functions = { .init = init_long, .deinit = deinit_long };
    #define TEST_VDLL_ARRAY_LEN (99)
    Vdll *dll = vdll_create(sizeof(long), &functions);
    assert(dll != NULL);
    assert(vdll_grow(dll, TEST_VDLL_ARRAY_LEN) == 0);

    long array[TEST_VDLL_ARRAY_LEN];
    long tmp;
    for(size_t i = 0; i < TEST_VDLL_ARRAY_LEN; i++){
        array[i] = rand();
        assert(vdll_set(dll, i, &(array[i])) == 0);
    }

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
    vdll_destroy(dll);

    return 0;
}

int varray_test(void){
    Varray *array = varray_create(sizeof(long));
    assert(array != NULL);
    assert(varray_grow(array, 4) == 0);
    assert(varray_len(array) == 4);
    assert(varray_grow(array, 1) == 0);
    assert(varray_len(array) == (4 + 1));

    long a = 1, b = 2, c = 3;
    assert(varray_set(array, 0, &a) == 0);
    assert(varray_set(array, 1, &b) == 0);
    assert(varray_set(array, 2, &c) == 0);

    long a_test, b_test, c_test;
    assert(varray_get(array, 0, &a_test) == 0);
    assert(varray_get(array, 1, &b_test) == 0);
    assert(varray_get(array, 2, &c_test) == 0);

    assert(a_test == a);
    assert(b_test == b);
    assert(c_test == c);

    assert(varray_shrink(array, 1) == 0);
    varray_destroy(array);

    return 0;
}

int vstack_test(void){
    Vstack *stack = vstack_create(sizeof(long), 3);
    assert(stack != NULL);
    assert(vstack_len(stack) == 0);
    assert(vstack_cap(stack) == 3);

    long a = 1, b = 2, c = 3;
    assert(vstack_push(stack, &a) == 0);
    assert(vstack_push(stack, &b) == 0);
    assert(vstack_push(stack, &c) == 0);
    assert(vstack_len(stack) == 3);
    assert(vstack_cap(stack) == 3);

    long a_test, b_test, c_test;
    assert(vstack_top(stack, &c_test) == 0);
    assert(c_test == c);
    assert(vstack_pop(stack, &c_test) == 0);
    assert(vstack_pop(stack, &b_test) == 0);
    assert(vstack_pop(stack, &a_test) == 0);
    assert(a_test == a);
    assert(b_test == b);
    assert(c_test == c);

    assert(vstack_len(stack) == 0);
    assert(vstack_cap(stack) == 3);
    assert(vstack_destroy(stack) == 0);
    
    return 0;
}

int vqueue_test_nooverwrite(void){
    Vqueue *queue = vqueue_create(sizeof(long), 3);
    assert(queue != NULL);
    assert(vqueue_len(queue) == 0);
    assert(vqueue_cap(queue) == 3);

    long a = 1, b = 2, c = 3;
    assert(vqueue_enqueue(queue, &a, false) == 0);
    assert(vqueue_enqueue(queue, &b, false) == 0);
    assert(vqueue_enqueue(queue, &c, false) == 0);
    assert(vqueue_len(queue) == 3);
    assert(vqueue_cap(queue) == 3);

    long a_test, b_test, c_test;
    assert(vqueue_front(queue, &a_test) == 0);
    assert(vqueue_back(queue, &c_test) == 0);
    assert(a_test == a);
    assert(c_test == c);
    assert(vqueue_dequeue(queue, &a_test) == 0);
    assert(vqueue_dequeue(queue, &b_test) == 0);
    assert(vqueue_dequeue(queue, &c_test) == 0);
    assert(a_test == a);
    assert(b_test == b);
    assert(c_test == c);

    assert(vqueue_len(queue) == 0);
    assert(vqueue_cap(queue) == 3);
    vqueue_destroy(queue);
    
    return 0;
}

int vqueue_test_overwrite(void){
    Vqueue *queue = vqueue_create(sizeof(long), 3);
    assert(queue != NULL);
    assert(vqueue_len(queue) == 0);
    assert(vqueue_cap(queue) == 3);

    long a = 1, b = 2, c = 3, d = 4;
    assert(vqueue_enqueue(queue, &a, true) == 0);
    assert(vqueue_enqueue(queue, &b, true) == 0);
    assert(vqueue_enqueue(queue, &c, true) == 0);
    assert(vqueue_enqueue(queue, &d, true) == 0);
    assert(vqueue_len(queue) == 3);
    assert(vqueue_cap(queue) == 3);

    long b_test, c_test, d_test;
    assert(vqueue_front(queue, &b_test) == 0);
    assert(vqueue_back(queue, &d_test) == 0);
    assert(b_test == b);
    assert(d_test == d);
    assert(vqueue_dequeue(queue, &b_test) == 0);
    assert(vqueue_dequeue(queue, &c_test) == 0);
    assert(b_test == b);
    assert(c_test == c);
    assert(vqueue_len(queue) == 1);
    assert(vqueue_cap(queue) == 3);

    long e = 5, f = 6;
    assert(vqueue_enqueue(queue, &e, true) == 0);
    assert(vqueue_enqueue(queue, &f, true) == 0);
    assert(vqueue_len(queue) == 3);
    assert(vqueue_cap(queue) == 3);

    long e_test, f_test;
    assert(vqueue_front(queue, &d_test) == 0);
    assert(vqueue_back(queue, &f_test) == 0);
    assert(d_test == d);
    assert(f_test == f);
    assert(vqueue_dequeue(queue, &d_test) == 0);
    assert(vqueue_dequeue(queue, &e_test) == 0);
    assert(vqueue_dequeue(queue, &f_test) == 0);
    assert(d_test == d);
    assert(e_test == e);
    assert(f_test == f);

    assert(vqueue_len(queue) == 0);
    assert(vqueue_cap(queue) == 3);
    vqueue_destroy(queue);
    
    return 0;
}

int vht_test(void){
    Vht *table = vht_create(sizeof(long), sizeof(char));
    assert(table != NULL);
    assert(vht_len(table) == 0);

    #define TEST_VHT_ARRAY_LEN (257)
    long keys[TEST_VHT_ARRAY_LEN];
    char vals[TEST_VHT_ARRAY_LEN];
    char ptrs[TEST_VHT_ARRAY_LEN];
    size_t i;
    for(i = 0; i < TEST_VHT_ARRAY_LEN; i++){
        keys[i] = rand();
        vals[i] = rand();
    }
    assert(vht_get_direct(table, &(keys[0])) == NULL);

    for(i = 0; i < TEST_VHT_ARRAY_LEN; i++){
        assert(vht_set(table, &(keys[i]), &(vals[i])) == 0);
    }
    assert(vht_len(table) == TEST_VHT_ARRAY_LEN);

    for(i = 0; i < TEST_VHT_ARRAY_LEN; i++){
        assert(vht_get(table, &(keys[i]), &(ptrs[i])) == 0);
        assert(ptrs[i] == vals[i]);
    }

    vht_destroy(table);
    return 0;
}

int fqueue_test(void){
    Fqueue *in = fqueue_create(999, "tests/fqueue_in.txt", "r");
    Fqueue *out = fqueue_create(999, "tests/fqueue_out.txt", "w");
    assert(in != NULL);
    assert(out != NULL);

    // Enqueue / Dequeue test
    char *dequeue_text = "testing dequeue\n";
    int dequeue_text_len = strlen(dequeue_text);
    char dequeue_text_check[999] = "";
    assert(fqueue_enqueue(in, dequeue_text, dequeue_text_len) == 0);
    assert(fqueue_dequeue(in, dequeue_text_check, fqueue_len(in)) == 0);
    assert(!strcmp(dequeue_text, dequeue_text_check));
    // now that all text content in the fqueue in has been consumed fold down
    assert(in->readCursor > 0);
    assert(in->writeCursor > 0);
    assert(fqueue_fold_down(in) == 0);
    assert(in->readCursor == 0);
    assert(in->writeCursor == 0);

    // Exchange / Rewind_Read_Cursor test
    char *exchange_text = "testing exchange\n";
    int exchange_text_len = strlen(exchange_text);
    char exchange_text_check[999] = "";
    assert(fqueue_enqueue(in, exchange_text, exchange_text_len) == 0);
    assert(fqueue_exchange(in, out, fqueue_len(in)) == 0);
    assert(fqueue_dequeue(out, exchange_text_check, fqueue_len(out)) == 0);
    assert(!strcmp(exchange_text, exchange_text_check));
    assert(fqueue_rewind_read_cursor(in, exchange_text_len) == 0);
    assert(fqueue_exchange(in, out, fqueue_len(in)) == 0);
    assert(fqueue_dequeue(out, exchange_text_check, fqueue_len(out)) == 0);
    assert(fqueue_fold_down(in) == 0);
    assert(fqueue_fold_down(out) == 0);

    // Rewind_Write_Cursor test
    char *rewind_text = "123456";
    int rewind_text_len = 6;
    char *rewind_text_partial = "123";
    char rewind_text_check[999] = "";
    int rewind_text_partial_len = 3;
    assert(fqueue_enqueue(in, rewind_text, rewind_text_len) == 0);
    assert(fqueue_rewind_write_cursor(in, rewind_text_len - rewind_text_partial_len) == 0);
    assert(fqueue_dequeue(in, rewind_text_check, rewind_text_partial_len) == 0);
    assert(!strcmp(rewind_text_partial, rewind_text_check));
    assert(fqueue_fold_down(in) == 0);

    // Fenqueue / Fdequeue test
    int fenqueue_text_len = 18;
    assert(fqueue_fenqueue(in, fenqueue_text_len) == 0);
    assert(fqueue_exchange(in, out, fenqueue_text_len) == 0);
    assert(fqueue_fdequeue(out, fqueue_len(out)) == 0);

    fqueue_destroy(in);
    fqueue_destroy(out);
    return 0;
}

int main(void){
    seed = time(NULL);
    printf("seed is %i\n", seed);
    srand(seed);
    vpool_test();
    vdll_test();
    varray_test();
    vstack_test();
    vqueue_test_nooverwrite();
    vqueue_test_overwrite();
    vht_test();
    fqueue_test();
}
