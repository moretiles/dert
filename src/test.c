#define _XOPEN_SOURCE (500)

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
#include <vht.h>
#include <fqueue.h>
#include <fmutex.h>
#include <fsemaphore.h>

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
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
    assert(*c = C_CONSTANT);
    assert(d != NULL);
    assert(*d = D_CONSTANT);
    assert(e != NULL);
    assert(*e = E_CONSTANT);
    assert(f != NULL);
    assert(*f = F_CONSTANT);
    assert((void*) c < pointer_literal_addition(arena->bytes, arena->bottom));
    assert((void*) f < pointer_literal_addition(arena->bytes, arena->bottom));
    assert(varena_arena_used(arena) >= FIRST_FRAME_SIZE + SECOND_FRAME_SIZE + THIRD_FRAME_SIZE);
    assert(varena_arena_unused(arena) != 0);
    assert(varena_arena_cap(arena) == 999);
    assert(varena_disclaim(&arena) == 0);

    assert(*b = B_CONSTANT);
    assert(varena_disclaim(&arena) == 0);

    assert(*a = A_CONSTANT);
    assert(varena_disclaim(&arena) == 0);

    varena_destroy(&arena);
    return 0;
}

int vpool_test(void) {
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
        char *a, *b;
        Tbuf *twin = tbuf_create(99);
        assert(twin != NULL);
        assert(tbuf_cap(twin) == 99);
        assert(tbuf_claim(twin, &a, &b) == 0);
        assert(a != NULL);
        assert(b != NULL);
        assert(strcpy(a, "123") != NULL);
        assert(strcpy(b, "456") != NULL);
        assert(!strcmp(a, "123"));
        assert(!strcmp(b, "456"));
        assert(tbuf_swap(twin) == 0);
        assert(tbuf_claim(twin, &a, &b) == 0);
        assert(a != NULL);
        assert(b != NULL);
        assert(!strcmp(a, "456"));
        assert(!strcmp(b, "123"));
        assert(tbuf_exchange(twin, &a, &b) == 0);
        assert(a != NULL);
        assert(b != NULL);
        assert(!strcmp(a, "456"));
        assert(!strcmp(b, "123"));
        assert(tbuf_exchange(twin, &a, &b) == 0);
        assert(a != NULL);
        assert(b != NULL);
        assert(!strcmp(a, "123"));
        assert(!strcmp(b, "456"));
        tbuf_destroy(twin);
    }

    {
        char *a, *b, *A, *B;
#define TBUF_TEST_NUM_TWINS (10)
        Tbuf *twins;
        void *memory = calloc(1, tbuf_advisev(TBUF_TEST_NUM_TWINS, 99));
        assert(memory != NULL);
        assert(tbuf_initv(TBUF_TEST_NUM_TWINS, &twins, memory, 99) == 0);

        for(size_t i = 0; i < TBUF_TEST_NUM_TWINS; i++) {
            assert(tbuf_exchange(&(twins[i]), &a, &b) == 0);
            assert(getrandom(a, 16, 0) == 16);
            assert(getrandom(b, 16, 0) == 16);
            A = a;
            B = b;
            assert(tbuf_exchange(&(twins[i]), &a, &b) == 0);
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

void *tpoolrr_test_function1(void *varg) {
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

void *tpoolrr_test_function2(void *varg) {
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
    Tpoolrr *pool;
    int *counter;
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
};

void *tpoolrr_test_worker3(void *varg) {
    int counter;
    void *retval;

    if(varg == NULL) {
        return "bad!";
    }

    struct tpoolrr_test_arg3 *arg = (struct tpoolrr_test_arg3*) varg;

    counter = __atomic_add_fetch(arg->counter, 1, __ATOMIC_SEQ_CST);
    if(counter == TPOOLRR_TEST_FUNCTION3_THREADS) {
        tpoolrr_handler_call(arg->pool, arg, &retval);
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
    pool = pool;

    return NULL;
}

int tpoolrr_test(void) {
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
        assert(tpoolrr_jobs_add(pool, tpoolrr_test_function1, (void *) &arg1, 0) == 0);
        assert(tpoolrr_jobs_add(pool, tpoolrr_test_function1, (void *) &arg2, 0) == 0);
        assert(tpoolrr_jobs_add(pool, tpoolrr_test_function1, (void *) &arg3, 0) == 0);

        // actual amount of active jobs/threads is unpredicatable
        // querying here so thread sanitizer can check for race conditions
        printf("PAUSING\n");
        assert(tpoolrr_pause(pool) == 0);
        printf("PAUSED\n");
        tpoolrr_jobs_queued(pool);
        tpoolrr_jobs_empty(pool);
        tpoolrr_jobs_cap(pool);
        tpoolrr_threads_active(pool);
        tpoolrr_threads_inactive(pool);
        tpoolrr_threads_total(pool);
        printf("RESUMING\n");
        assert(tpoolrr_resume(pool) == 0);
        printf("RESUMED\n");
        tpoolrr_jobs_queued(pool);
        tpoolrr_jobs_empty(pool);
        tpoolrr_jobs_cap(pool);
        tpoolrr_threads_active(pool);
        tpoolrr_threads_inactive(pool);
        tpoolrr_threads_total(pool);

        printf("JOINING\n");
        assert(tpoolrr_join(pool) == 0);
        printf("JOINED\n");
        printf("%i == %i\n", src1, dest1);
        printf("queues: %lu, %lu, %lu\n",
               aqueue_len(&(pool->job_queues[0])),
               aqueue_len(&(pool->job_queues[1])),
               aqueue_len(&(pool->job_queues[2])));
        assert(src1 == dest1);
        assert(src2 == dest2);
        assert(src3 == dest3);
        tpoolrr_destroy(pool);
    }

    {
        char src1[999] = "foo", src2[999] = "bar", src3[999] = "baz";
        char dest1[999], dest2[999], dest3[999];
        Tpoolrr_fn functions[] = {tpoolrr_test_function2, tpoolrr_test_function2, tpoolrr_test_function2};
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
        assert(tpoolrr_jobs_addall(pool, 3, functions, args, 0) == 0);
        printf("STOPPING\n");
        assert(tpoolrr_stop_unsafe(pool) == 0);
        printf("STOPPED\n");
        tpoolrr_destroy(pool);
        */

        pool = tpoolrr_create(TPOOLRR_TEST_FUNCTION2_THREADS, TPOOLRR_TEST_FUNCTION2_JOBS);
        assert(tpoolrr_jobs_addall(pool, 3, functions, args, 0) == 0);
        printf("JOINING\n");
        assert(tpoolrr_join(pool) == 0);
        printf("JOINED\n");
        printf("%s == %s\n", src1, dest1);
        printf("queues: %lu, %lu, %lu\n",
               aqueue_len(&(pool->job_queues[0])),
               aqueue_len(&(pool->job_queues[1])),
               aqueue_len(&(pool->job_queues[2])));
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
        struct tpoolrr_test_arg3 arg = { pool, &counter, &done_yet, &done_cond };
        assert(tpoolrr_jobs_assign(pool, tpoolrr_test_worker3, (void *) &arg, 0) == 0);

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

    vht_destroy(table);
    return 0;
}

int fqueue_test(void) {
    Fqueue *in = fqueue_create(999, "tests/fqueue_in.txt", "r");
    Fqueue *out = fqueue_create(999, "tests/fqueue_out.txt", "w");
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
};

#define FSEMAPHORE_SEM_MAX (10)
void *fsemaphore_test_function(void *varg) {
    struct fsemaphore_test_function_arg *arg;
    assert(varg != NULL);
    arg = varg;

    assert(fsemaphore_wait(arg->sem) == 0);
    __atomic_fetch_add(&(arg->array[arg->index % FSEMAPHORE_SEM_MAX]), arg->index, __ATOMIC_ACQ_REL);
    usleep(100);
    assert(fsemaphore_post(arg->sem) == 0);

    return NULL;
}

int fsemaphore_test(void) {
    void *retval;
    pthread_t threads[3 * FSEMAPHORE_SEM_MAX];
    int array[3 * FSEMAPHORE_SEM_MAX];
    struct fsemaphore_test_function_arg args[3 * FSEMAPHORE_SEM_MAX];
    Fsemaphore *sem = fsemaphore_create(1, 1);
    assert(sem != NULL);

    memset(array, 0, 3 * FSEMAPHORE_SEM_MAX * sizeof(int));
    for(int i = 0; i < 3 * FSEMAPHORE_SEM_MAX; i++) {
        args[i].sem = sem;
        args[i].array = array;
        args[i].index = i;
        pthread_create(&(threads[i]), NULL, fsemaphore_test_function, &(args[i]));
    }

    for(int i = 0; i < 3 * FSEMAPHORE_SEM_MAX; i++) {
        pthread_join(threads[i], &retval);
    }

    for(int i = 0; i < FSEMAPHORE_SEM_MAX; i++) {
        assert(array[i] == (3 * (i + FSEMAPHORE_SEM_MAX)));
    }

    fsemaphore_destroy(sem);
    return 0;
}

int main(void) {
    seed = time(NULL);
    printf("seed is %i\n", seed);
    srand(seed);

    varena_test();
    vpool_test();
    vdll_test();
    tbuf_test();
    varray_test();
    vstack_test();
    vqueue_test_nooverwrite();
    vqueue_test_overwrite();
    aqueue_test_nooverwrite();
    mpscqueue_test_nooverwrite();
    vht_test();
    tpoolrr_test();
    fmutex_test();
    fsemaphore_test();

    fqueue_test();
}
