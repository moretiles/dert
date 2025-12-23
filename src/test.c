#define _XOPEN_SOURCE (500)
#define _GNU_SOURCE (1)

#include <assert.h>
#include <pointerarith.h>

#include <vpool.h>
#include <varena.h>
#include <varena_priv.h>
#include <vdll.h>
#include <tbuf.h>
#include <varray.h>
#include <vstack.h>
#include <vqueue.h>
#include <aqueue.h>
#include <mpscqueue.h>
#include <tpoolrr.h>
#include <gtpoolrr.h>
#include <vht.h>
#include <fqueue.h>
#include <fmutex.h>
#include <fsemaphore.h>
#include <tree_T.h>
#include <tree_iterator.h>

#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/random.h>

int seed;

int init_long(void *ptr) {
    if(ptr == NULL) {
        return 1;
    }

    long *long_ptr = ptr;
    *long_ptr = 1;
    return 0;
}

int deinit_long(void *ptr) {
    if(ptr == NULL) {
        return 1;
    }

    long *long_ptr = ptr;
    *long_ptr = 0;
    return 0;
}

int varena_test(void) {
#define FIRST_FRAME_SIZE (0x04)
#define SECOND_FRAME_SIZE (0x04)
#define THIRD_FRAME_SIZE (0x10)
#define A_CONSTANT (0x01234567)
#define B_CONSTANT (0x89ABCDEF)
#define C_CONSTANT (0x00112233)
#define D_CONSTANT (0x44556677)
#define E_CONSTANT (0x8899AABB)
#define F_CONSTANT (0xCCDDEEFF)

    int32_t *a, *b, *c, *d, *e, *f;
    Varena *arena = varena_create(999);
    assert(arena != NULL);
    assert(varena_arena_used(arena) == 0);
    assert(varena_arena_unused(arena) == 999);
    assert(varena_arena_cap(arena) == 999);

    assert(varena_claim(&arena, FIRST_FRAME_SIZE) == 0);
    assert(varena_frame_used(arena) == 0);
    assert(varena_frame_unused(arena) >= FIRST_FRAME_SIZE);
    a = varena_alloc(&arena, sizeof(int32_t));
    assert(a != NULL);
    assert(varena_frame_used(arena) >= FIRST_FRAME_SIZE);
    assert(varena_frame_unused(arena) == 0);
    assert((void*) a < pointer_literal_addition(arena->bytes, arena->bottom));
    *a = A_CONSTANT;

    assert(varena_claim(&arena, SECOND_FRAME_SIZE) == 0);
    assert(varena_frame_used(arena) == 0);
    assert(varena_frame_unused(arena) >= SECOND_FRAME_SIZE);
    b = varena_alloc(&arena, sizeof(int32_t));
    assert(b != NULL);
    assert(varena_frame_used(arena) >= SECOND_FRAME_SIZE);
    assert(varena_frame_unused(arena) == 0);
    assert((void*) b < pointer_literal_addition(arena->bytes, arena->bottom));
    *b = B_CONSTANT;

    assert(varena_claim(&arena, THIRD_FRAME_SIZE) == 0);
    assert(varena_frame_used(arena) == 0);
    assert(varena_frame_unused(arena) >= THIRD_FRAME_SIZE);
    c = varena_alloc(&arena, sizeof(int32_t));
    *c = C_CONSTANT;
    d = varena_alloc(&arena, sizeof(int32_t));
    *d = D_CONSTANT;
    e = varena_alloc(&arena, sizeof(int32_t));
    *e = E_CONSTANT;
    f = varena_alloc(&arena, sizeof(int32_t));
    *f = F_CONSTANT;
    assert(varena_frame_used(arena) >= THIRD_FRAME_SIZE);
    assert(varena_frame_unused(arena) == 0);

    assert(c != NULL);
    assert((*c = C_CONSTANT));
    assert(d != NULL);
    assert((*d = D_CONSTANT));
    assert(e != NULL);
    assert((*e = E_CONSTANT));
    assert(f != NULL);
    assert((*f = F_CONSTANT));
    assert((void*) c < pointer_literal_addition(arena->bytes, arena->bottom));
    assert((void*) f < pointer_literal_addition(arena->bytes, arena->bottom));
    assert(varena_arena_used(arena) >= FIRST_FRAME_SIZE + SECOND_FRAME_SIZE + THIRD_FRAME_SIZE);
    assert(varena_arena_unused(arena) != 0);
    assert(varena_arena_cap(arena) == 999);
    assert(varena_disclaim(&arena) == 0);

    assert((*b = B_CONSTANT));
    assert(varena_disclaim(&arena) == 0);

    assert((*a = A_CONSTANT));
    assert(varena_disclaim(&arena) == 0);

    varena_destroy(&arena);
    return 0;
}

int vpool_test(void) {
    Vpool *longs;
    long *a, *b, *c, *d, *e;

    {
        longs = vpool_create(3, sizeof(long), VPOOL_KIND_STATIC);
        assert(longs != NULL);

        a = vpool_alloc(longs);
        b = vpool_alloc(longs);
        c = vpool_alloc(longs);
        d = vpool_alloc(longs);
        assert(a != NULL && b != NULL && c != NULL);
        assert(a != b && b != c && a != c);
        assert(d == NULL);

        *a = 1;
        *b = 2;
        *c = 3;

        vpool_dealloc(longs, a);
        vpool_dealloc(longs, b);
        vpool_dealloc(longs, c);

        a = vpool_alloc(longs);
        b = vpool_alloc(longs);
        c = vpool_alloc(longs);

        vpool_dealloc(longs, a);
        vpool_dealloc(longs, c);

        vpool_destroy(longs);
    }

    {
        longs = vpool_create(1, sizeof(long), VPOOL_KIND_GUIDED);
        assert(longs != NULL);

        a = vpool_alloc(longs);
        assert(a != NULL);
        b = vpool_alloc(longs);
        assert(b == NULL);
        assert(vpool_full);

        size_t memory_size = vpool_advise(2, sizeof(long));
        void *memory = calloc(1, memory_size);
        assert(memory != NULL);
        assert(vpool_guided_extend(longs, memory, memory_size) == 0);
        b = vpool_alloc(longs);
        c = vpool_alloc(longs);
        d = vpool_alloc(longs);
        assert(b != NULL && c != NULL);
        assert(d == NULL);

        *a = 1;
        *b = 2;
        *c = 3;

        vpool_dealloc(longs, a);
        vpool_dealloc(longs, b);
        vpool_dealloc(longs, c);

        void *memory2 = calloc(1, memory_size);
        assert(memory2 != NULL);
        assert(vpool_guided_extend(longs, memory2, memory_size) == 0);

        a = vpool_alloc(longs);
        b = vpool_alloc(longs);
        c = vpool_alloc(longs);
        d = vpool_alloc(longs);
        e = vpool_alloc(longs);

        vpool_dealloc(longs, a);
        vpool_dealloc(longs, c);

        vpool_destroy(longs);
        free(memory2);
        free(memory);
    }

    {
        longs = vpool_create(1, sizeof(long), VPOOL_KIND_DYNAMIC);
        assert(longs != NULL);

        a = vpool_alloc(longs);
        b = vpool_alloc(longs);
        c = vpool_alloc(longs);
        d = vpool_alloc(longs);
        e = vpool_alloc(longs);
        assert(a != NULL && b != NULL && c != NULL && d != NULL && e != NULL);

        *a = 1;
        *b = 2;
        *c = 3;
        *d = 4;
        *e = 5;

        vpool_dealloc(longs, a);
        vpool_dealloc(longs, b);
        vpool_dealloc(longs, c);
        vpool_dealloc(longs, d);
        vpool_dealloc(longs, e);

        a = vpool_alloc(longs);
        b = vpool_alloc(longs);
        c = vpool_alloc(longs);
        d = vpool_alloc(longs);
        e = vpool_alloc(longs);

        vpool_dealloc(longs, a);
        vpool_dealloc(longs, c);

        vpool_destroy(longs);
    }

    return 0;
}

int vdll_test(void) {
    Vdll_functions functions = { .init = init_long, .deinit = deinit_long };
#define TEST_VDLL_ARRAY_LEN (99)
    Vdll *dll = vdll_create(sizeof(long), &functions);
    assert(dll != NULL);
    assert(vdll_grow(dll, TEST_VDLL_ARRAY_LEN) == 0);

    long array[TEST_VDLL_ARRAY_LEN];
    long tmp;
    for(size_t i = 0; i < TEST_VDLL_ARRAY_LEN; i++) {
        array[i] = rand();
        assert(vdll_set(dll, i, &(array[i])) == 0);
    }

    for(size_t i = 0; i < TEST_VDLL_ARRAY_LEN * 10; i++) {
        size_t pos = rand() % TEST_VDLL_ARRAY_LEN;
        array[pos] = rand();
        assert(vdll_set(dll, pos, &(array[pos])) == 0);
    }

    for(size_t i = 0; i < TEST_VDLL_ARRAY_LEN * 100; i++) {
        size_t pos = rand() % TEST_VDLL_ARRAY_LEN;
        assert(vdll_get(dll, pos, &tmp) == 0);
        assert(tmp == array[pos]);
    }

    assert(vdll_len(dll) == TEST_VDLL_ARRAY_LEN);
    assert(vdll_shrink(dll, 1 + (TEST_VDLL_ARRAY_LEN / 2)) == 0);
    vdll_destroy(dll);

    return 0;
}

int tbuf_test(void) {
    {
#define TBUF_TEST_BUF_SIZE (99)
        const char *TBUF_TEST_STR_A = "0123456789";
        const char * TBUF_TEST_STR_B = "ABCDEF";

        char *a, *b;
        Tbuf *twin = tbuf_create(TBUF_TEST_BUF_SIZE);
        assert(tbuf_cap(twin) == TBUF_TEST_BUF_SIZE);

        assert(twin != NULL);
        assert(tbuf_cap(twin) == TBUF_TEST_BUF_SIZE);
        a = twin->A;
        b = twin->B;
        assert(a != NULL);
        assert(b != NULL);
        assert(strcpy(a, TBUF_TEST_STR_A) != NULL);
        twin->A_len += strlen(TBUF_TEST_STR_A) + 1;
        twin->B_len += strlen(TBUF_TEST_STR_B) + 1;
        assert(strcpy(b, TBUF_TEST_STR_B) != NULL);
        assert(tbuf_A_unused(twin) == TBUF_TEST_BUF_SIZE - strlen(TBUF_TEST_STR_A) - 1);
        assert(tbuf_B_unused(twin) == TBUF_TEST_BUF_SIZE - strlen(TBUF_TEST_STR_B) - 1);
        assert(!strcmp(a, TBUF_TEST_STR_A));
        assert(!strcmp(b, TBUF_TEST_STR_B));

        assert(tbuf_swap(twin) == 0);
        a = twin->A;
        b = twin->B;
        assert(a != NULL);
        assert(b != NULL);
        assert(!strcmp(a, TBUF_TEST_STR_B));
        assert(!strcmp(b, TBUF_TEST_STR_A));

        a = twin->A;
        b = twin->B;
        assert(a != NULL);
        assert(b != NULL);
        assert(!strcmp(a, TBUF_TEST_STR_B));
        assert(!strcmp(b, TBUF_TEST_STR_A));
        assert(tbuf_swap(twin) == 0);
        a = twin->A;
        b = twin->B;
        assert(a != NULL);
        assert(b != NULL);
        assert(!strcmp(a, TBUF_TEST_STR_A));
        assert(!strcmp(b, TBUF_TEST_STR_B));

        tbuf_destroy(twin);
    }

    {
        char *a, *b, *A, *B;
#define TBUF_TEST_NUM_TWINS (10)
        Tbuf *twins;
        void *memory = calloc(1, tbuf_advisev(TBUF_TEST_NUM_TWINS, TBUF_TEST_BUF_SIZE));
        assert(memory != NULL);
        assert(tbuf_initv(TBUF_TEST_NUM_TWINS, &twins, memory, TBUF_TEST_BUF_SIZE) == 0);

        for(size_t i = 0; i < TBUF_TEST_NUM_TWINS; i++) {
            a = tbuf_A(&(twins[i]));
            b = tbuf_B(&(twins[i]));
            assert(getrandom(a, 16, 0) == 16);
            assert(getrandom(b, 16, 0) == 16);
            A = a;
            B = b;
            assert(tbuf_swap(&(twins[i])) == 0);
            a = tbuf_A(&(twins[i]));
            b = tbuf_B(&(twins[i]));
            assert(!strcmp(a, B));
            assert(!strcmp(b, A));
            tbuf_deinit(&(twins[i]));
        }
        free(&(twins[0]));
    }

    return 0;
}

int varray_test(void) {
    Varray *array = varray_create(sizeof(long));
    assert(array != NULL);
    assert(varray_len(array) == 0);
    assert(varray_cap(array) == 0);
    assert(varray_realloc(array, 4) == 0);
    assert(varray_len(array) == 4);
    assert(varray_cap(array) >= 4);
    assert(varray_realloc(array, 5) == 0);
    assert(varray_len(array) == 5);
    assert(varray_cap(array) >= 5);

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

    assert(varray_realloc(array, 4) == 0);
    assert(varray_len(array) == 4);
    assert(varray_cap(array) >= 4);

    assert(varray_resize(array, 0) == 0);
    assert(varray_len(array) == 0);
    assert(varray_cap(array) >= 3);
    assert(varray_get(array, 0, &a_test) != 0);
    assert(varray_get(array, 1, &b_test) != 0);
    assert(varray_get(array, 2, &c_test) != 0);
    assert(varray_resize(array, 3) == 0);
    assert(varray_len(array) == 3);
    assert(varray_cap(array) >= 3);
    assert(varray_get(array, 0, &a_test) == 0);
    assert(varray_get(array, 1, &b_test) == 0);
    assert(varray_get(array, 2, &c_test) == 0);
    assert(a_test == a);
    assert(b_test == b);
    assert(c_test == c);

    varray_destroy(array);

    return 0;
}

int vstack_test(void) {

    {
        Vstack *stack;
        stack = vstack_create(sizeof(long), 3);
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
    }

    {
#define VSTACK_TEST_NUM_STACKS (20)
        Vstack *stacks;
        void *memory;
        long a, b, c, a_test, b_test, c_test;

        memory = calloc(1, vstack_advisev(VSTACK_TEST_NUM_STACKS, sizeof(long), 3));
        assert(memory != NULL);
        assert(vstack_initv(VSTACK_TEST_NUM_STACKS, &stacks, memory, sizeof(long), 3) == 0);

        for(size_t i = 0; i < VSTACK_TEST_NUM_STACKS; i++) {
            a = rand();
            assert(vstack_push(&(stacks[i]), &a) == 0);
            b = rand();
            assert(vstack_push(&(stacks[i]), &b) == 0);
            c = rand();
            assert(vstack_push(&(stacks[i]), &c) == 0);

            assert(vstack_pop(&(stacks[i]), &c_test) == 0);
            assert(c == c_test);
            assert(vstack_pop(&(stacks[i]), &b_test) == 0);
            assert(b == b_test);
            assert(vstack_pop(&(stacks[i]), &a_test) == 0);
            assert(a == a_test);
        }

        for(size_t i = 0; i < VSTACK_TEST_NUM_STACKS; i++) {
            vstack_deinit(&(stacks[i]));
        }
        free(stacks);
    }

    return 0;
}

int vqueue_test_nooverwrite(void) {
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

int vqueue_test_overwrite(void) {
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

int vqueue_test_some_nooverwrite(void) {
    Vqueue *queue = vqueue_create(sizeof(long), 3);
    size_t num_enqueued = 0;
    size_t num_dequeued = 0;
    assert(queue != NULL);
    assert(vqueue_len(queue) == 0);
    assert(vqueue_cap(queue) == 3);

    long two_longs_src[2] = { 1, 2 };
    long two_longs_dest[2] = { 0, 0 };
    long two_more_longs_src[2] = { 3, 4 };
    long two_more_longs_dest[2] = { 0, 67 };
    assert(vqueue_enqueue_some(queue, &num_enqueued, 2, two_longs_src, false) == 0);
    assert(num_enqueued == 2);
    assert(vqueue_dequeue_some(queue, &num_dequeued, 2, two_longs_dest) == 0);
    assert(num_dequeued == 2);
    assert(two_longs_dest[0] == 1);
    assert(two_longs_dest[1] == 2);
    assert(vqueue_enqueue_some(queue, &num_enqueued, 2, two_longs_src, false) == 0);
    assert(num_enqueued == 2);
    assert(vqueue_enqueue_some(queue, &num_enqueued, 2, two_more_longs_src, false) == EXFULL);
    assert(num_enqueued == 1);
    assert(vqueue_dequeue_some(queue, &num_dequeued, 2, two_longs_dest) == 0);
    assert(num_dequeued == 2);
    assert(two_longs_dest[0] == 1);
    assert(two_longs_dest[1] == 2);
    assert(vqueue_dequeue_some(queue, &num_dequeued, 2, two_more_longs_dest) == ENODATA);
    assert(num_dequeued == 1);
    assert(two_more_longs_dest[0] == 3);
    assert(two_more_longs_dest[1] == 67);

    free(queue);
    return 0;
}

int vqueue_test_some_overwrite(void) {
    Vqueue *queue = vqueue_create(sizeof(long), 3);
    size_t num_enqueued = 0;
    size_t num_dequeued = 0;
    assert(queue != NULL);
    assert(vqueue_len(queue) == 0);
    assert(vqueue_cap(queue) == 3);

    long two_longs_src[2] = { 1, 2 };
    long two_longs_dest[2] = { 0, 0 };
    long two_more_longs_src[2] = { 3, 4 };
    long two_more_longs_dest[2] = { 0, 67 };
    assert(vqueue_enqueue_some(queue, &num_enqueued, 2, two_longs_src, true) == 0);
    assert(num_enqueued == 2);
    assert(vqueue_dequeue_some(queue, &num_dequeued, 2, two_longs_dest) == 0);
    assert(num_dequeued == 2);
    assert(two_longs_dest[0] == 1);
    assert(two_longs_dest[1] == 2);
    assert(vqueue_enqueue_some(queue, &num_enqueued, 2, two_longs_src, true) == 0);
    assert(num_enqueued == 2);
    assert(vqueue_enqueue_some(queue, &num_enqueued, 2, two_more_longs_src, true) == 0);
    assert(num_enqueued == 2);
    assert(vqueue_dequeue_some(queue, &num_dequeued, 2, two_longs_dest) == 0);
    assert(num_dequeued == 2);
    assert(two_longs_dest[0] == 2);
    assert(two_longs_dest[1] == 3);
    assert(vqueue_dequeue_some(queue, &num_dequeued, 2, two_more_longs_dest) == ENODATA);
    assert(num_dequeued == 1);
    assert(two_more_longs_dest[0] == 4);
    assert(two_more_longs_dest[1] == 67);

    free(queue);
    return 0;
}

int aqueue_test_nooverwrite(void) {
    Aqueue *queue = aqueue_create(sizeof(long), 3);
    assert(queue != NULL);
    assert(aqueue_len(queue) == 0);
    assert(aqueue_cap(queue) == 3);

    long a = 1, b = 2, c = 3;
    assert(aqueue_enqueue(queue, &a) == 0);
    assert(aqueue_enqueue(queue, &b) == 0);
    assert(aqueue_enqueue(queue, &c) == 0);
    assert(aqueue_len(queue) == 3);
    assert(aqueue_cap(queue) == 3);

    long a_test, b_test, c_test;
    assert(aqueue_front(queue, &a_test) == 0);
    assert(a_test == a);
    assert(aqueue_dequeue(queue, &a_test) == 0);
    assert(aqueue_dequeue(queue, &b_test) == 0);
    assert(aqueue_dequeue(queue, &c_test) == 0);
    assert(a_test == a);
    assert(b_test == b);
    assert(c_test == c);

    assert(aqueue_len(queue) == 0);
    assert(aqueue_cap(queue) == 3);
    aqueue_destroy(queue);

    return 0;
}

int aqueue_test_some(void) {
    Aqueue *queue = aqueue_create(sizeof(long), 3);
    size_t num_enqueued = 0;
    size_t num_dequeued = 0;
    assert(queue != NULL);
    assert(aqueue_len(queue) == 0);
    assert(aqueue_cap(queue) == 3);

    long two_longs_src[2] = { 1, 2 };
    long two_longs_dest[2] = { 0, 0 };
    long two_more_longs_src[2] = { 3, 4 };
    long two_more_longs_dest[2] = { 0, 67 };
    assert(aqueue_enqueue_some(queue, &num_enqueued, 2, two_longs_src) == 0);
    assert(num_enqueued == 2);
    assert(aqueue_dequeue_some(queue, &num_dequeued, 2, two_longs_dest) == 0);
    assert(num_dequeued == 2);
    assert(two_longs_dest[0] == 1);
    assert(two_longs_dest[1] == 2);
    assert(aqueue_enqueue_some(queue, &num_enqueued, 2, two_longs_src) == 0);
    assert(num_enqueued == 2);
    assert(aqueue_enqueue_some(queue, &num_enqueued, 2, two_more_longs_src) == EXFULL);
    assert(num_enqueued == 1);
    assert(aqueue_dequeue_some(queue, &num_dequeued, 2, two_longs_dest) == 0);
    assert(num_dequeued == 2);
    assert(two_longs_dest[0] == 1);
    assert(two_longs_dest[1] == 2);
    assert(aqueue_dequeue_some(queue, &num_dequeued, 2, two_more_longs_dest) == ENODATA);
    assert(num_dequeued == 1);
    assert(two_more_longs_dest[0] == 3);
    assert(two_more_longs_dest[1] == 67);

    free(queue);
    return 0;
}

int mpscqueue_test_nooverwrite(void) {
    Mpscqueue *queue = mpscqueue_create(sizeof(long), 3);
    assert(queue != NULL);
    assert(mpscqueue_len(queue) == 0);
    assert(mpscqueue_cap(queue) == 3);

    long a = 1, b = 2, c = 3;
    assert(mpscqueue_enqueue(queue, &a) == 0);
    assert(mpscqueue_enqueue(queue, &b) == 0);
    assert(mpscqueue_enqueue(queue, &c) == 0);
    assert(mpscqueue_len(queue) == 3);
    assert(mpscqueue_cap(queue) == 3);

    long a_test, b_test, c_test;
    assert(mpscqueue_front(queue, &a_test) == 0);
    assert(a_test == a);
    assert(mpscqueue_dequeue(queue, &a_test) == 0);
    assert(mpscqueue_dequeue(queue, &b_test) == 0);
    assert(mpscqueue_dequeue(queue, &c_test) == 0);
    assert(a_test == a);
    assert(b_test == b);
    assert(c_test == c);

    assert(mpscqueue_len(queue) == 0);
    assert(mpscqueue_cap(queue) == 3);
    mpscqueue_destroy(queue);

    return 0;
}

#define TPOOLRR_TEST_FUNCTION1_THREADS (5)
#define TPOOLRR_TEST_FUNCTION1_JOBS (3)
struct tpoolrr_test_arg1 {
    int *src;
    int *dest;
};

void *tpoolrr_test_function1(Tpoolrr *pool, void *varg) {
    (void) pool;
    if(varg == NULL) {
        return "bad!";
    }

    struct tpoolrr_test_arg1 *arg = (struct tpoolrr_test_arg1*) varg;
    if(arg->src == NULL || arg->dest == NULL) {
        return "bad!";
    }

    __atomic_store_n(arg->dest, *(arg->src), __ATOMIC_SEQ_CST);

    return NULL;
}

#define TPOOLRR_TEST_FUNCTION2_THREADS (5)
#define TPOOLRR_TEST_FUNCTION2_JOBS (3)
struct tpoolrr_test_arg2 {
    char *src;
    char *dest;
};

void *tpoolrr_test_function2(Tpoolrr *pool, void *varg) {
    (void) pool;
    if(varg == NULL) {
        return "bad!";
    }

    struct tpoolrr_test_arg2 *arg = (struct tpoolrr_test_arg2*) varg;
    if(arg->src == NULL || arg->dest == NULL) {
        return "bad!";
    }

    strcpy(arg->dest, arg->src);

    return NULL;
}

#define TPOOLRR_TEST_FUNCTION3_THREADS (1)
#define TPOOLRR_TEST_FUNCTION3_JOBS (3)
struct tpoolrr_test_arg3 {
    int *counter;
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
};

void *tpoolrr_test_worker3(Tpoolrr *pool, void *varg) {
    int counter;
    void *retval;

    if(varg == NULL) {
        return "bad!";
    }

    struct tpoolrr_test_arg3 *arg = (struct tpoolrr_test_arg3*) varg;

    counter = __atomic_add_fetch(arg->counter, 1, __ATOMIC_SEQ_CST);
    if(counter == TPOOLRR_TEST_FUNCTION3_THREADS) {
        tpoolrr_handler_call(pool, arg, &retval);
    }

    return NULL;
}

void *tpoolrr_test_handler3(Tpoolrr *pool, void *varg) {
    if(varg == NULL) {
        return "bad!";
    }

    struct tpoolrr_test_arg3 *arg = varg;
    pthread_mutex_lock(arg->mutex);
    pthread_cond_signal(arg->cond);
    pthread_mutex_unlock(arg->mutex);

    // do nothing with pool
    (void)(pool);

    return NULL;
}

int tpoolrr_sort_done_jobs(const void *src1, const void *src2) {
    if(src1 == NULL || src2 == NULL) {
        return 0;
    }

    struct tpoolrr_job *job1 = (struct tpoolrr_job *) src1;
    struct tpoolrr_job *job2 = (struct tpoolrr_job *) src2;

    if(job1->user_tag < job2->user_tag) {
        return -1;
    } else if(job1->user_tag == job2->user_tag) {
        return 0;
    } else {
        return 1;
    }
}

int tpoolrr_test(void) {
    uint64_t user_tag = 0;
    Tpoolrr *pool;
    printf("Allocating %lu bytes\n", tpoolrr_advise(100, 100));

    {
        pool = tpoolrr_create(TPOOLRR_TEST_FUNCTION1_THREADS, TPOOLRR_TEST_FUNCTION1_JOBS);
        assert(pool != NULL);
        int src1 = 1, src2 = 2, src3 = 3;
        int dest1 = 0, dest2 = 0, dest3 = 0;
        struct tpoolrr_test_arg1 arg1 = {(int*) &src1, (int*) &dest1};
        struct tpoolrr_test_arg1 arg2 = {(int*) &src2, (int*) &dest2};
        struct tpoolrr_test_arg1 arg3 = {(int*) &src3, (int*) &dest3};
        assert(tpoolrr_jobs_add(pool, user_tag++, tpoolrr_test_function1, (void *) &arg1, 0) == 0);
        assert(tpoolrr_jobs_add(pool, user_tag++, tpoolrr_test_function1, (void *) &arg2, 0) == 0);
        assert(tpoolrr_jobs_add(pool, user_tag++, tpoolrr_test_function1, (void *) &arg3, 0) == 0);

        // actual amount of active jobs/threads is unpredicatable
        // querying here so thread sanitizer can check for race conditions
        printf("PAUSING\n");
        assert(tpoolrr_pause(pool) == 0);
        printf("PAUSED\n");
        tpoolrr_submissions_queued(pool);
        tpoolrr_submissions_empty(pool);
        tpoolrr_submissions_cap(pool);
        tpoolrr_threads_active(pool);
        tpoolrr_threads_inactive(pool);
        tpoolrr_threads_total(pool);
        printf("RESUMING\n");
        assert(tpoolrr_resume(pool) == 0);
        printf("RESUMED\n");
        tpoolrr_submissions_queued(pool);
        tpoolrr_submissions_empty(pool);
        tpoolrr_submissions_cap(pool);
        tpoolrr_threads_active(pool);
        tpoolrr_threads_inactive(pool);
        tpoolrr_threads_total(pool);

        printf("JOINING\n");
        assert(tpoolrr_join(pool) == 0);
        printf("JOINED\n");
        struct tpoolrr_job done_jobs[3];
        assert(tpoolrr_completions_popall(pool, done_jobs, 3) == 0);
        assert(src1 == dest1);
        assert(src2 == dest2);
        assert(src3 == dest3);
        qsort(done_jobs, 3, sizeof(struct tpoolrr_job), tpoolrr_sort_done_jobs);
        for(size_t i = 0; i < 3; i++) {
            assert(done_jobs[i].user_tag == i);
        }
        tpoolrr_destroy(pool);
    }

    {
        char src1[999] = "foo", src2[999] = "bar", src3[999] = "baz";
        char dest1[999], dest2[999], dest3[999];
        Tpoolrr_fn functions[3] = {tpoolrr_test_function2, tpoolrr_test_function2, tpoolrr_test_function2};
        uint64_t user_tags[3];
        user_tags[0] = user_tag++;
        user_tags[1] = user_tag++;
        user_tags[2] = user_tag++;
        struct tpoolrr_test_arg2 arg1 = {(char*) src1, (char*) dest1};
        struct tpoolrr_test_arg2 arg2 = {(char*) src2, (char*) dest2};
        struct tpoolrr_test_arg2 arg3 = {(char*) src3, (char*) dest3};
        void *args[] = {(void *) &arg1, (void *) &arg2, (void *) &arg3};
        /*
        // Because stop_unsafe is dangerous there is often a use-after-free
        // cannot avoid this
        // have confirmed in gdb that threads are killed by stop_unsafe

        pool = tpoolrr_create(TPOOLRR_TEST_FUNCTION2_THREADS, TPOOLRR_TEST_FUNCTION2_JOBS);
        assert(pool != NULL);
        assert(tpoolrr_jobs_addall(pool, 3, user_tags, functions, args, 0) == 0);
        printf("STOPPING\n");
        assert(tpoolrr_stop_unsafe(pool) == 0);
        printf("STOPPED\n");
        tpoolrr_destroy(pool);
        */

        pool = tpoolrr_create(TPOOLRR_TEST_FUNCTION2_THREADS, TPOOLRR_TEST_FUNCTION2_JOBS);
        user_tags[0] = user_tag++;
        user_tags[1] = user_tag++;
        user_tags[2] = user_tag++;
        assert(tpoolrr_jobs_addall(pool, 3, user_tags, functions, args, 0) == 0);
        printf("JOINING\n");
        assert(tpoolrr_join(pool) == 0);
        printf("JOINED\n");
        printf("%s == %s\n", src1, dest1);
        printf("queues: %lu, %lu, %lu\n",
               aqueue_len(&(pool->job_submission_queues[0])),
               aqueue_len(&(pool->job_submission_queues[1])),
               aqueue_len(&(pool->job_submission_queues[2])));
        assert(!strcmp(src1, dest1));
        assert(!strcmp(src2, dest2));
        assert(!strcmp(src3, dest3));
        tpoolrr_destroy(pool);
    }

    {
        int counter = 0;
        pthread_mutex_t done_yet = { 0 };
        pthread_cond_t done_cond = { 0 };
        assert(pthread_mutex_init(&done_yet, NULL) == 0);
        assert(pthread_mutex_lock(&done_yet) == 0);
        assert(pthread_cond_init(&done_cond, NULL) == 0);
        // mutex will be unlocked once counter has finished counting

        pool = tpoolrr_create(TPOOLRR_TEST_FUNCTION3_THREADS, TPOOLRR_TEST_FUNCTION3_JOBS);
        assert(pool != NULL);
        assert(tpoolrr_handler_update(pool, tpoolrr_test_handler3) == 0);
        struct tpoolrr_test_arg3 arg = { &counter, &done_yet, &done_cond };
        assert(tpoolrr_jobs_assign(pool, user_tag++, tpoolrr_test_worker3, (void *) &arg, 0) == 0);

        assert(pthread_cond_wait(&done_cond, &done_yet) == 0);
        assert(pthread_mutex_unlock(&done_yet) == 0);
        printf("JOINING\n");
        tpoolrr_join(pool);
        printf("JOINED\n");
        printf("counter: %i\n", counter);
        assert(counter == TPOOLRR_TEST_FUNCTION3_THREADS);
        tpoolrr_destroy(pool);
    }

    return 0;
}

struct gtpoolrr_test_arg1 {
    int n;
};

// Does simple print, yields to parent to do nop, does another print
void *gtpoolrr_test1(volatile Gtpoolrr *pool, volatile Greent *green_thread, volatile void *varg) {
    if(pool == NULL || green_thread == NULL || varg == NULL) {
        return "bad";
    }

    volatile struct gtpoolrr_test_arg1 *arg = (struct gtpoolrr_test_arg1*) varg;

    printf("function called: %d\n", arg->n);
    uint64_t ret = greent_do_nop(green_thread);
    printf("yield returned: %d\n", (int) ret);

    return NULL;
}

struct gtpoolrr_test_arg2 {
    char *buf;
    char *filename;
};

// Cats out the first 63 bytes of a file
void *gtpoolrr_test2(volatile Gtpoolrr *pool, volatile Greent *green_thread, volatile void *varg) {
    volatile struct gtpoolrr_test_arg2 *arg;
    volatile int fd;
    volatile char *buf;
    volatile char *filename;
    if(pool == NULL || green_thread == NULL || varg == NULL) {
        goto gtpoolrr_test2_end;
    }

    arg = (volatile struct gtpoolrr_test_arg2*) varg;
    buf = arg->buf;
    filename = arg->filename;

    mode_t mode = 0600;
    greent_do_open(green_thread, filename, O_RDONLY, mode);
    fd = green_thread->completion.res;
    if(fd <= 0) {
        printf("Error opening: %s\n", filename);
        goto gtpoolrr_test2_end;
    }

    greent_do_readt(green_thread, fd, buf, 256 - 1, 0, 1, 0);
    const int read_res = green_thread->completion.res;
    if(read_res < 0) {
        errno = -read_res;
        printf("Error reading %s:", filename);
        perror("");
    }
    buf[green_thread->completion.res] = 0;

    printf("%s: %s", filename, buf);

gtpoolrr_test2_end:
    if(fd > 0) {
        greent_do_close(green_thread, fd);
        if(green_thread->completion.res != 0) {
            // should never happen
            assert(false);
        }
    }
    return "";
}

struct gtpoolrr_test_arg3 {
    char *buf;
    char *in_filename;
    char *out_filename;
};

// want to make sure greent_do_readt and greent_do_write work together
void *gtpoolrr_test3(volatile Gtpoolrr *pool, volatile Greent *green_thread, volatile void *varg) {
    volatile int in_fd = 0;
    volatile int out_fd = 0;
    if(pool == NULL || green_thread == NULL || varg == NULL) {
        return NULL;
    }

    volatile struct gtpoolrr_test_arg3 *arg = (volatile struct gtpoolrr_test_arg3*) varg;
    volatile char *buf = arg->buf;
    volatile char *in_filename = arg->in_filename;
    volatile char *out_filename = arg->out_filename;

    mode_t mode = 0600;
    puts("start open file for reading");
    greent_do_open(green_thread, in_filename, O_RDONLY, mode);
    puts("end open file for reading");
    in_fd = green_thread->completion.res;
    if(in_fd <= 0) {
        printf("Error opening: %s\n", in_filename);
        assert(false);
        goto gtpoolrr_test3_end;
    }

    puts("start open file for writing");
    greent_do_open(green_thread, out_filename, O_WRONLY | O_CREAT | O_TRUNC, mode);
    puts("end open file for writing");
    out_fd = green_thread->completion.res;
    if(out_fd <= 0) {
        printf("Error opening: %s\n", out_filename);
        assert(false);
        goto gtpoolrr_test3_end;
    }

    puts("start read file");
    greent_do_read(green_thread, in_fd, buf, 256 - 1, 0);
    puts("end read file");
    const int read_res = green_thread->completion.res;
    if(read_res < 0) {
        errno = -read_res;
        printf("Error reading %s:", in_filename);
        perror("");
        assert(false);
        goto gtpoolrr_test3_end;
    }
    buf[green_thread->completion.res] = 0;

    puts("start write file");
    greent_do_write(green_thread, out_fd, buf, read_res, 0);
    puts("end write file");
    const int write_res = green_thread->completion.res;
    if(write_res < 0) {
        errno = -read_res;
        printf("Error writing %s:", out_filename);
        perror("");
        assert(false);
        goto gtpoolrr_test3_end;
    }

gtpoolrr_test3_end:
    if(in_fd > 0) {
        puts("start close read file");
        greent_do_close(green_thread, in_fd);
        puts("end close read file");
        if(green_thread->completion.res != 0) {
            // should never happen
            assert(false);
        }
    }

    if(out_fd > 0) {
        puts("start close written file");
        greent_do_close(green_thread, out_fd);
        puts("end close written file");
        if(green_thread->completion.res != 0) {
            // should never happen
            assert(false);
        }
    }
    return "";
}

int gtpoolrr_test(void) {
    {
        struct gtpoolrr_job *jobs[10] = { 0 };
        size_t num_obtained;
        Gtpoolrr *pool1;
        pool1 = gtpoolrr_create(1,1);
        assert(pool1 != NULL);
        assert(gtpoolrr_pause(pool1) == 0);
        assert(gtpoolrr_resume(pool1) == 0);
        assert(0 == gtpoolrr_sbs_get(pool1, jobs, &num_obtained, 1));
        gtpoolrr_sbs_set_tag(jobs[0], 8080);
        gtpoolrr_sbs_set_function(jobs[0], gtpoolrr_test1);
        struct gtpoolrr_test_arg1 arg = (struct gtpoolrr_test_arg1) {
            .n = 55
        };
        gtpoolrr_sbs_set_arg(jobs[0], &arg);
        //gtpoolrr_sbs_set_expiration(jobs[0], 0);
        assert(gtpoolrr_sbs_push_direct(pool1, 0, jobs[0]) == 0);

        gtpoolrr_join(pool1);
        assert(gtpoolrr_cps_popall(pool1, jobs, 1) == 0);
        gtpoolrr_cps_ack(pool1, &(jobs[0]), 1);
        gtpoolrr_destroy(pool1);
    }

    {
        Gtpoolrr *pool2;
        size_t num_pushed;
        pool2 = gtpoolrr_create(1,6);
        assert(pool2 != NULL);

        char bufs[2][256];
        char *filenames[2] = { "./tests/DIR.txt", "./obj/dir.txt"};
        struct gtpoolrr_test_arg2 args[2] = { 0 };
        args[0] = (struct gtpoolrr_test_arg2) {
            .buf = bufs[0], .filename = filenames[0]
        };
        args[1] = (struct gtpoolrr_test_arg2) {
            .buf = bufs[1], .filename = filenames[1]
        };

        struct gtpoolrr_job *jobs[10] = { 0 };
        assert(0 == gtpoolrr_sbs_get(pool2, jobs, &num_pushed, 2));
        gtpoolrr_sbs_set_tag(jobs[0], 111);
        gtpoolrr_sbs_set_tag(jobs[1], 222);
        gtpoolrr_sbs_set_functions(jobs, 2, gtpoolrr_test2);
        gtpoolrr_sbs_set_arg(jobs[0], &(args[0]));
        gtpoolrr_sbs_set_arg(jobs[1], &(args[1]));
        gtpoolrr_sbs_set_expirations(jobs, 2, 10 * (1LLU << 30)); // about 10 seconds
        assert(gtpoolrr_sbs_pushall(pool2, &num_pushed, 2, jobs) == 0);

        assert(gtpoolrr_cps_popall(pool2, jobs, 2) == 0);
        gtpoolrr_cps_ack(pool2, &(jobs[0]), 2);
        gtpoolrr_destroy(pool2);
    }

    {
        Gtpoolrr *pool3;
        size_t num_pushed;
        pool3 = gtpoolrr_create(1,3);
        assert(pool3 != NULL);
        char bufs[3][256];
        char *in_filenames[3] = {
            "tests/gtpoolrr/readtwritet_reference.txt",
            "tests/gtpoolrr/readtwritet_reference.txt",
            "tests/gtpoolrr/readtwritet_reference.txt"
        };
        char *out_filenames[3] = {
            "./tests/gtpoolrr/readtwritet_a_out.txt",
            "./tests/gtpoolrr/readtwritet_b_out.txt",
            "./tests/gtpoolrr/readtwritet_c_out.txt"
        };
        struct gtpoolrr_test_arg3 arg0 = (struct gtpoolrr_test_arg3) {
            .buf = bufs[0],
            .in_filename = in_filenames[0],
            .out_filename = out_filenames[0]
        };
        struct gtpoolrr_test_arg3 arg1 = (struct gtpoolrr_test_arg3) {
            .buf = bufs[1],
            .in_filename = in_filenames[1],
            .out_filename = out_filenames[1]
        };
        struct gtpoolrr_test_arg3 arg2 = (struct gtpoolrr_test_arg3) {
            .buf = bufs[2],
            .in_filename = in_filenames[2],
            .out_filename = out_filenames[2]
        };

        struct gtpoolrr_job *jobs[3] = { 0 };
        assert(gtpoolrr_sbs_get(pool3, jobs, &num_pushed, 3) == 0);
        gtpoolrr_sbs_set_tag(jobs[0], 1 * 1111);
        gtpoolrr_sbs_set_tag(jobs[1], 2 * 1111);
        gtpoolrr_sbs_set_tag(jobs[2], 3 * 1111);
        gtpoolrr_sbs_set_functions(jobs, 3, gtpoolrr_test3);
        gtpoolrr_sbs_set_arg(jobs[0], &arg0);
        gtpoolrr_sbs_set_arg(jobs[1], &arg1);
        gtpoolrr_sbs_set_arg(jobs[2], &arg2);

        assert(gtpoolrr_sbs_pushall_direct(pool3, 0, &num_pushed, 3, jobs) == 0);
        assert(gtpoolrr_cps_popall(pool3, jobs, 3) == 0);
        gtpoolrr_join(pool3);
        gtpoolrr_destroy(pool3);
    }

    return 0;
}

int vht_test(void) {
    Vht *table = vht_create(sizeof(long), sizeof(char));
    assert(table != NULL);
    assert(vht_len(table) == 0);

#define TEST_VHT_ARRAY_LEN (257)
    long keys[TEST_VHT_ARRAY_LEN];
    char vals[TEST_VHT_ARRAY_LEN];
    char ptrs[TEST_VHT_ARRAY_LEN];
    size_t i;
    for(i = 0; i < TEST_VHT_ARRAY_LEN; i++) {
        keys[i] = rand();
        vals[i] = rand();
    }
    assert(vht_get_direct(table, &(keys[0])) == NULL);

    for(i = 0; i < TEST_VHT_ARRAY_LEN; i++) {
        assert(vht_set(table, &(keys[i]), &(vals[i])) == 0);
    }
    assert(vht_len(table) == TEST_VHT_ARRAY_LEN);

    for(i = 0; i < TEST_VHT_ARRAY_LEN; i++) {
        assert(vht_get(table, &(keys[i]), &(ptrs[i])) == 0);
        assert(ptrs[i] == vals[i]);
    }

    int64_t vals_sum = 0;
    long dest_key = 0;
    char dest_val = 0;
    Vht_iterator iterator;
    int64_t vht_sum = 0;
    int res;

    for(i = 0; i < TEST_VHT_ARRAY_LEN; i++) {
        vals_sum += vals[i];
    }

    assert(vht_iterate_start(table, &iterator) == 0);
    while((res = vht_iterate_next(table, &iterator, &dest_key, &dest_val)) != ENODATA) {
        assert(res == 0);

        vht_sum += dest_val;
    }
    assert(vals_sum == vht_sum);

    vht_destroy(table);
    return 0;
}

int fqueue_test(void) {
    Fqueue *in = fqueue_create(999, "tests/fqueue/fqueue_in.txt", "r");
    Fqueue *out = fqueue_create(999, "tests/fqueue/fqueue_out.txt", "w");
    assert(in != NULL);
    assert(out != NULL);

    // Enqueue / Dequeue test
    char *dequeue_text = "testing dequeue\n";
    size_t dequeue_text_len = strlen(dequeue_text);
    char dequeue_text_check[999] = "";
    assert(fqueue_enqueue(in, dequeue_text, dequeue_text_len) == 0);
    assert(fqueue_dequeue(in, dequeue_text_check, fqueue_used(in)) == 0);
    assert(!strcmp(dequeue_text, dequeue_text_check));
    // now that all text content in the fqueue in has been consumed fold down
    assert(fqueue_prev(in) == dequeue_text_len);
    assert(fqueue_used(in) == 0);
    assert(in->readCursor > 0);
    assert(in->writeCursor > 0);
    assert(fqueue_fold_down(in) == 0);
    assert(in->readCursor == 0);
    assert(in->writeCursor == 0);

    // Exchange / Rewind_Read_Cursor test
    char *exchange_text = "testing exchange\n";
    size_t exchange_text_len = strlen(exchange_text);
    char exchange_text_check[999] = "";
    assert(fqueue_enqueue(in, exchange_text, exchange_text_len) == 0);
    assert(fqueue_exchange(in, out, fqueue_used(in)) == 0);
    assert(fqueue_dequeue(out, exchange_text_check, fqueue_used(out)) == 0);
    assert(!strcmp(exchange_text, exchange_text_check));
    assert(fqueue_rewind_read_cursor(in, exchange_text_len) == 0);
    assert(fqueue_exchange(in, out, fqueue_used(in)) == 0);
    assert(fqueue_dequeue(out, exchange_text_check, fqueue_used(out)) == 0);
    assert(fqueue_fold_down(in) == 0);
    assert(fqueue_fold_down(out) == 0);

    // Rewind_Write_Cursor test
    char *rewind_text = "123456";
    size_t rewind_text_len = 6;
    char *rewind_text_partial = "123";
    char rewind_text_check[999] = "";
    size_t rewind_text_partial_len = 3;
    assert(fqueue_enqueue(in, rewind_text, rewind_text_len) == 0);
    assert(fqueue_rewind_write_cursor(in, rewind_text_len - rewind_text_partial_len) == 0);
    assert(fqueue_dequeue(in, rewind_text_check, rewind_text_partial_len) == 0);
    assert(!strcmp(rewind_text_partial, rewind_text_check));
    assert(fqueue_fold_down(in) == 0);

    // Fenqueue / Fdequeue test
    size_t fenqueue_text_len = 18;
    assert(fqueue_fenqueue(in, fenqueue_text_len) == 0);
    assert(fqueue_exchange(in, out, fenqueue_text_len) == 0);
    assert(fqueue_fdequeue(out, fqueue_used(out)) == 0);

    fqueue_destroy(in);
    fqueue_destroy(out);
    return 0;
}

struct fmutex_test_worker_arg {
    Fmutex *mutex;
    int counter;
};

void *fmutex_test_worker(void *varg) {
    struct fmutex_test_worker_arg *arg;
    assert(varg != NULL);

    arg = (struct fmutex_test_worker_arg *) varg;
    assert(fmutex_lock(arg->mutex) == 0);
    arg->counter += 1;
    assert(fmutex_unlock(arg->mutex) == 0);

    return NULL;
}

int fmutex_test(void) {
    void *retval;
    struct fmutex_test_worker_arg arg;
    pthread_t threads[200];

    Fmutex *mutex = fmutex_create();
    assert(mutex != NULL);

    arg.mutex = mutex;
    arg.counter = 0;
    for(size_t i = 0; i < 200; i++) {
        pthread_create(&threads[i], NULL, fmutex_test_worker, &arg);
    }

    for(size_t i = 0; i < 200; i++) {
        pthread_join(threads[i], &retval);
    }

    fmutex_destroy(mutex);
    return 0;
}

struct fsemaphore_test_function_arg {
    Fsemaphore *sem;
    int *array;
    uint64_t index;
    bool do_post;
};

#define FSEMAPHORE_SEM_MAX (10)
void *fsemaphore_test_function(void *varg) {
    struct fsemaphore_test_function_arg *arg;
    assert(varg != NULL);
    arg = varg;

    assert(fsemaphore_wait(arg->sem) == 0);
    __atomic_fetch_add(&(arg->array[arg->index % FSEMAPHORE_SEM_MAX]), arg->index, __ATOMIC_ACQ_REL);
    usleep(100);
    if(arg->do_post) {
        assert(fsemaphore_post(arg->sem) == 0);
    }

    return NULL;
}

int fsemaphore_test(void) {
    void *retval;
    pthread_t threads[3 * FSEMAPHORE_SEM_MAX];
    int array[3 * FSEMAPHORE_SEM_MAX];
    struct fsemaphore_test_function_arg args[3 * FSEMAPHORE_SEM_MAX];

    // worker threads call fsemaphore_post
    {
        Fsemaphore *sem = fsemaphore_create(1, 1);
        assert(sem != NULL);

        memset(array, 0, 3 * FSEMAPHORE_SEM_MAX * sizeof(int));
        for(int i = 0; i < 3 * FSEMAPHORE_SEM_MAX; i++) {
            args[i].sem = sem;
            args[i].array = array;
            args[i].index = i;
            args[i].do_post = true;
            pthread_create(&(threads[i]), NULL, fsemaphore_test_function, &(args[i]));
        }

        for(int i = 0; i < 3 * FSEMAPHORE_SEM_MAX; i++) {
            pthread_join(threads[i], &retval);
        }

        for(int i = 0; i < FSEMAPHORE_SEM_MAX; i++) {
            assert(array[i] == (3 * (i + FSEMAPHORE_SEM_MAX)));
        }

        fsemaphore_destroy(sem);
    }

    // main thread calls fsemaphore_reset
    {
        Fsemaphore *sem = fsemaphore_create(1, 1);
        assert(sem != NULL);

        memset(array, 0, 3 * FSEMAPHORE_SEM_MAX * sizeof(int));
        for(int i = 0; i < 3 * FSEMAPHORE_SEM_MAX; i++) {
            args[i].sem = sem;
            args[i].array = array;
            args[i].index = i;
            args[i].do_post = false;
            pthread_create(&(threads[i]), NULL, fsemaphore_test_function, &(args[i]));
        }

        for(int i = 0; i < 3 * FSEMAPHORE_SEM_MAX; i++) {
            struct timespec timeout = { 0 };

            do {
                clock_gettime(CLOCK_REALTIME, &timeout);
                timeout.tv_nsec += 5 * (1 << 20);
                fsemaphore_reset(sem);
            } while(pthread_timedjoin_np(threads[i], &retval, &timeout) != 0);
        }

        for(int i = 0; i < FSEMAPHORE_SEM_MAX; i++) {
            assert(array[i] == (3 * (i + FSEMAPHORE_SEM_MAX)));
        }

        fsemaphore_destroy(sem);
    }

    return 0;
}

int tree_T_stringify(char *dest, Tree_node *src, size_t cap) {
    if(dest == NULL || src == NULL || cap == 0) {
        return EINVAL;
    }

    int res = snprintf(dest, cap, "%lu:%lu", (size_t) src, (size_t) src->val);

    return res > 0 ? 0 : res;
}

void tree_puts_debug(Tree_tree *tree, char *string, char *filename) {
    printf("== %s ==\n", string);
    puts("= Pre-Order =");
    assert(tree_puts_pre(tree, tree_T_stringify, filename) == 0);
    puts("= In-Order =");
    assert(tree_puts_in(tree, tree_T_stringify, NULL) == 0);
    puts("= Post-Order =");
    assert(tree_puts_post(tree, tree_T_stringify, NULL) == 0);
    puts("= Breadth First Search =");
    assert(tree_puts_bfs(tree, tree_T_stringify, NULL) == 0);
    puts("");
    puts("");
    puts("");
}

int tree_T_test(void) {
    Tree_tree *tree;
    Tree_tree *tree_backup;

#define TREE_T_TEST_MAX_NODE (99)
    {
        tree = tree_create(TREE_T_TEST_MAX_NODE);
        tree_backup = malloc(tree_advise(TREE_T_TEST_MAX_NODE));
        assert(tree_backup != NULL);

        // perfectly balanced tree with height of 2
        // basic insertion
        {
            // height = 0
            assert(tree_insert(tree, 50) == 0);

            // height = 1
            assert(tree_insert(tree, 30) == 0);
            assert(tree_insert(tree, 70) == 0);

            // height = 2
            assert(tree_insert(tree, 20) == 0);
            assert(tree_insert(tree, 40) == 0);
            assert(tree_insert(tree, 60) == 0);
            assert(tree_insert(tree, 80) == 0);
        }

        // lookup node
        {
            Tree_node *root_node, *min_node, *max_node;
            root_node = tree_lookup(tree, 50);
            assert(root_node != NULL);
            assert(root_node->val == 50);
            min_node = tree_lookup(tree, 20);
            assert(min_node != NULL);
            assert(min_node->val == 20);
            max_node = tree_lookup(tree, 80);
            assert(max_node != NULL);
            assert(max_node->val == 80);

            Tree_node *first_range_node, *smallest_range_node, *largest_range_node;
            first_range_node = tree_lookup_range(tree, 20, 30, TREE_LOOKUP_FIRST);
            assert(first_range_node != NULL);
            assert(first_range_node->val == 30);
            smallest_range_node = tree_lookup_range(tree, 20, 30, TREE_LOOKUP_SMALLEST);
            assert(smallest_range_node != NULL);
            assert(smallest_range_node->val == 20);
            largest_range_node = tree_lookup_range(tree, 20, 30, TREE_LOOKUP_LARGEST);
            assert(largest_range_node != NULL);
            assert(largest_range_node->val == 30);

            min_node = tree_node_min(tree, tree->root);
            assert(min_node != NULL);
            assert(min_node->val == 20);
            max_node = tree_node_max(tree, tree->root);
            assert(max_node != NULL);
            assert(max_node->val == 80);
        }

        // iteration
        {
            tree_puts_debug(tree, "Perfectly balanced tree", "tests/vtree/graphs/simple/0001_initial.png");
        }

        // delete
        {
            assert(memcpy(tree_backup, tree, tree_advise(TREE_T_TEST_MAX_NODE)) == tree_backup);
            assert(tree_delete(tree, 50) == 0);
            assert(tree_delete(tree, 30) == 0);
            tree_puts_debug(tree, "First set of deletes", "tests/vtree/graphs/simple/0002_firstdelete.png");


            assert(memcpy(tree, tree_backup, tree_advise(TREE_T_TEST_MAX_NODE)) == tree);
            assert(tree_delete(tree, 50) == 0);
            assert(tree_delete(tree, 70) == 0);
            assert(tree_delete(tree, 80) == 0);
            tree_puts_debug(tree, "Second set of deletes", "tests/vtree/graphs/simple/0003_secondelete.png");


            assert(memcpy(tree, tree_backup, tree_advise(TREE_T_TEST_MAX_NODE)) == tree);
            assert(tree_delete(tree, 50) == 0);
            assert(tree_delete(tree, 30) == 0);
            assert(tree_delete(tree, 20) == 0);
            assert(tree_delete(tree, 70) == 0);
            assert(tree_delete(tree, 80) == 0);
            assert(tree_delete(tree, 40) == 0);
            assert(tree_delete(tree, 60) == 0);
            tree_puts_debug(tree, "Only remaining node is 60", "tests/vtree/graphs/simple/0003_secondelete.png");
            assert(memcpy(tree, tree_backup, tree_advise(TREE_T_TEST_MAX_NODE)) == tree);
        }

        // update
        {
            assert(memcpy(tree_backup, tree, tree_advise(TREE_T_TEST_MAX_NODE)) == tree_backup);
            assert(tree_update(tree, 90, 50) == 0);
            tree_puts_bfs(tree, tree_T_stringify, "tests/vtree/graphs/simple/0004_afterfirstupdate.png");
            assert(tree_update(tree, 45, 70) == 0);
            tree_puts_bfs(tree, tree_T_stringify, "tests/vtree/graphs/simple/0005_aftersecondupdate.png");
            assert(tree_update(tree, 5,  40) == 0);
            tree_puts_bfs(tree, tree_T_stringify, "tests/vtree/graphs/simple/0006_afterthirdupdate.png");
            assert(memcpy(tree, tree_backup, tree_advise(TREE_T_TEST_MAX_NODE)) == tree);
        }

        // many
        {
            uint8_t insert_vals[4] = { 15, 30, 45, 60 };
            uint8_t delete_vals[4] = { 15, 30, 45, 60 };
            uint8_t update_vals_new[4] = { 30, 40, 50, 60 };
            uint8_t update_vals_old[4] = { 20, 30, 60, 80 };
            assert(memcpy(tree_backup, tree, tree_advise(TREE_T_TEST_MAX_NODE)) == tree_backup);
            tree_puts_bfs(tree, tree_T_stringify, "tests/vtree/graphs/simple/0100_before_many.png");
            assert(tree_insert_many(tree, insert_vals, 4, sizeof(uint8_t), true) == 0);
            tree_puts_bfs(tree, tree_T_stringify, "tests/vtree/graphs/simple/0101_after_insert_many.png");
            assert(tree_delete_many(tree, delete_vals, 4) == 0);
            tree_puts_bfs(tree, tree_T_stringify, "tests/vtree/graphs/simple/0102_after_delete_many.png");
            assert(tree_update_many(tree, update_vals_new, update_vals_old, 4, false) == 0);
            tree_puts_bfs(tree, tree_T_stringify, "tests/vtree/graphs/simple/0103_after_update_many.png");
            assert(memcpy(tree, tree_backup, tree_advise(TREE_T_TEST_MAX_NODE)) == tree);
        }

        tree_destroy(tree);
        free(tree_backup);
    }

    {
#define TREE_T_TEST_VALUES_LEN (60)
        const uint8_t values[TREE_T_TEST_VALUES_LEN] = { 42, 80, 42, 43, 10, 82, 95, 67, 37, 74, 88, 50, 14, 73, 98, 79, 7, 59, 37, 28, 5, 75, 94, 56, 4, 31, 1, 31, 91, 5, 45, 71, 82, 55, 72, 87, 69, 82, 60, 2, 61, 88, 66, 50, 25, 25, 42, 27, 6, 26, 92, 54, 10, 79, 61, 66, 92, 8, 79, 17 };
        tree = tree_create(TREE_T_TEST_MAX_NODE);
        tree_backup = malloc(tree_advise(TREE_T_TEST_MAX_NODE));
        assert(tree_backup != NULL);

        // insertion
        for(size_t i = 0; i < TREE_T_TEST_VALUES_LEN; i++) {
            assert(tree_insert(tree, values[i]) == 0);
        }

        // lookup node
        {
            Tree_node *root_node, *min_node, *max_node;
            root_node = tree_lookup(tree, 42);
            assert(root_node != NULL);
            assert(root_node->val == 42);
            min_node = tree_lookup(tree, 1);
            assert(min_node != NULL);
            assert(min_node->val == 1);
            max_node = tree_lookup(tree, 98);
            assert(max_node != NULL);
            assert(max_node->val == 98);

            min_node = tree_node_min(tree, tree->root);
            assert(min_node != NULL);
            assert(min_node->val == 1);
            max_node = tree_node_max(tree, tree->root);
            assert(max_node != NULL);
            assert(max_node->val == 98);

            assert(tree_reachable(tree) == true);
        }

        // iteration
        {
            // bfs is working
            tree_puts_bfs(tree, tree_T_stringify, "tests/vtree/graphs/bigtree/0001_initial.png");

            // preorder gets into infinite loop
            //tree_puts_pre(tree, tree_T_stringify, NULL);

            // inorder misses 98 (the largest value on the tree)
            //tree_puts_post(tree, tree_T_stringify, NULL);

            // not sure about postorder
            //tree_puts_in(tree, tree_T_stringify, NULL);
        }

        // delete
        {
            //assert(memcpy(tree_backup, tree, tree_advise(TREE_T_TEST_MAX_NODE)) == tree_backup);
            //assert(tree_delete(tree, 50) == 0);
            //assert(tree_delete(tree, 30) == 0);
            //tree_puts_debug(tree, "First set of deletes", "tests/vtree/graphs/simple/0002_firstdelete.png");


            //assert(memcpy(tree, tree_backup, tree_advise(TREE_T_TEST_MAX_NODE)) == tree);
            //assert(tree_delete(tree, 56) == 0);
            //assert(tree_delete(tree, 31) == 0);
            //assert(tree_delete(tree, 82) == 0);
            //assert(tree_min(tree) != NULL);
            //assert(tree_max(tree) != NULL);
            //assert(tree_reachable(tree) == true);
            //tree_puts_debug(tree, "Second set of deletes", "tests/vtree/graphs/simple/0003_secondelete.png");


            //assert(memcpy(tree, tree_backup, tree_advise(TREE_T_TEST_MAX_NODE)) == tree);
            //assert(tree_delete(tree, 50) == 0);
            //assert(tree_delete(tree, 30) == 0);
            //assert(tree_delete(tree, 20) == 0);
            //assert(tree_delete(tree, 70) == 0);
            //assert(tree_delete(tree, 80) == 0);
            //assert(tree_delete(tree, 40) == 0);
            //assert(tree_delete(tree, 60) == 0);
            //tree_puts_debug(tree, "Only remaining node is 60", "tests/vtree/graphs/simple/0003_secondelete.png");
            //assert(memcpy(tree, tree_backup, tree_advise(TREE_T_TEST_MAX_NODE)) == tree);
        }

        // update
        /*
        {
            assert(tree_update(tree, 90, 50) == 0);
            assert(tree_update(tree, 45, 70) == 0);
            assert(tree_update(tree, 5,  40) == 0);
        }
        */

        tree_destroy(tree);
        free(tree_backup);
    }

    return 0;
}

int main(void) {
    seed = time(NULL);
    printf("seed is %i\n", seed);
    srand(seed);

    /*
    varena_test();
    vpool_test();
    vdll_test();
    tbuf_test();
    varray_test();
    vstack_test();
    vqueue_test_nooverwrite();
    vqueue_test_overwrite();
    vqueue_test_some_nooverwrite();
    vqueue_test_some_overwrite();
    aqueue_test_nooverwrite();
    aqueue_test_some();
    mpscqueue_test_nooverwrite();
    vht_test();
    tpoolrr_test();
    gtpoolrr_test();
    fmutex_test();
    fsemaphore_test();
    tree_T_test();
    fqueue_test();
    */

    gtpoolrr_test();
}
