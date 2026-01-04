//#include <aqueue.h>
//#include <tpoolrr.h>

//#include <gtest/gtest.h>

namespace {
#define TPOOLRR_TEST_FUNCTION1_THREADS (5)
#define TPOOLRR_TEST_FUNCTION1_JOBS (3)
struct tpoolrr_test_arg1 {
    int *src;
    int *dest;
};

void *tpoolrr_test_function1(Tpoolrr *pool, void *varg) {
    (void) pool;
    if(varg == NULL) {
        return (void *) "bad!";
    }

    struct tpoolrr_test_arg1 *arg = (struct tpoolrr_test_arg1*) varg;
    if(arg->src == NULL || arg->dest == NULL) {
        return (void *) "bad!";
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
        return (void *) "bad!";
    }

    struct tpoolrr_test_arg2 *arg = (struct tpoolrr_test_arg2*) varg;
    if(arg->src == NULL || arg->dest == NULL) {
        return (void *) "bad!";
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
        return (void *) "bad!";
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
        return (void *) "bad!";
    }

    struct tpoolrr_test_arg3 *arg = (struct tpoolrr_test_arg3 *) varg;
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

TEST(tpoolrr, 1) {
    uint64_t user_tag = 0;
    Tpoolrr *pool;

    pool = tpoolrr_create(TPOOLRR_TEST_FUNCTION1_THREADS, TPOOLRR_TEST_FUNCTION1_JOBS);
    ASSERT_NE(nullptr, pool);
    int src1 = 1, src2 = 2, src3 = 3;
    int dest1 = 0, dest2 = 0, dest3 = 0;
    struct tpoolrr_test_arg1 arg1 = {(int*) &src1, (int*) &dest1};
    struct tpoolrr_test_arg1 arg2 = {(int*) &src2, (int*) &dest2};
    struct tpoolrr_test_arg1 arg3 = {(int*) &src3, (int*) &dest3};
    ASSERT_EQ(0, tpoolrr_jobs_add(pool, user_tag++, tpoolrr_test_function1, (void *) &arg1, 0));
    ASSERT_EQ(0, tpoolrr_jobs_add(pool, user_tag++, tpoolrr_test_function1, (void *) &arg2, 0));
    ASSERT_EQ(0, tpoolrr_jobs_add(pool, user_tag++, tpoolrr_test_function1, (void *) &arg3, 0));

    // actual amount of active jobs/threads is unpredicatable
    // querying here so thread sanitizer can check for race conditions
    MT_LOG("PAUSING");
    ASSERT_EQ(0, tpoolrr_pause(pool));
    MT_LOG("PAUSED");
    tpoolrr_submissions_queued(pool);
    tpoolrr_submissions_empty(pool);
    tpoolrr_submissions_cap(pool);
    tpoolrr_threads_active(pool);
    tpoolrr_threads_inactive(pool);
    tpoolrr_threads_total(pool);
    MT_LOG("RESUMING");
    ASSERT_EQ(0, tpoolrr_resume(pool));
    MT_LOG("RESUMED");
    tpoolrr_submissions_queued(pool);
    tpoolrr_submissions_empty(pool);
    tpoolrr_submissions_cap(pool);
    tpoolrr_threads_active(pool);
    tpoolrr_threads_inactive(pool);
    tpoolrr_threads_total(pool);

    MT_LOG("JOINING");
    ASSERT_EQ(0, tpoolrr_join(pool));
    MT_LOG("JOINED");
    struct tpoolrr_job done_jobs[3];
    ASSERT_EQ(0, tpoolrr_completions_popall(pool, done_jobs, 3));
    ASSERT_EQ(src1, dest1);
    ASSERT_EQ(src2, dest2);
    ASSERT_EQ(src3, dest3);
    qsort(done_jobs, 3, sizeof(struct tpoolrr_job), tpoolrr_sort_done_jobs);
    for(size_t i = 0; i < 3; i++) {
        ASSERT_EQ(i, done_jobs[i].user_tag);
    }
    tpoolrr_destroy(pool);
}

TEST(tpoolrr, 2) {
    uint64_t user_tag = 0;
    Tpoolrr *pool;

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
    ASSERT_NE(pool, NULL);
    ASSERT_EQ(0, tpoolrr_jobs_addall(pool, 3, user_tags, functions, args, 0));
    MT_LOG("STOPPING");
    ASSERT_EQ(0, tpoolrr_stop_unsafe(pool));
    MT_LOG("STOPPED");
    tpoolrr_destroy(pool);
    */

    pool = tpoolrr_create(TPOOLRR_TEST_FUNCTION2_THREADS, TPOOLRR_TEST_FUNCTION2_JOBS);
    user_tags[0] = user_tag++;
    user_tags[1] = user_tag++;
    user_tags[2] = user_tag++;
    ASSERT_EQ(0, tpoolrr_jobs_addall(pool, 3, user_tags, functions, args, 0));
    MT_LOG("JOINING");
    ASSERT_EQ(0, tpoolrr_join(pool));
    MT_LOG("JOINED");
    MT_LOG("%s == %s", src1, dest1);
    MT_LOG("queues: %lu, %lu, %lu",
           aqueue_len(&(pool->job_submission_queues[0])),
           aqueue_len(&(pool->job_submission_queues[1])),
           aqueue_len(&(pool->job_submission_queues[2])));
    ASSERT_STREQ(src1, dest1);
    ASSERT_STREQ(src2, dest2);
    ASSERT_STREQ(src3, dest3);
    tpoolrr_destroy(pool);
}

TEST(tpoolrr, 3) {
    uint64_t user_tag = 0;
    Tpoolrr *pool;

    int counter = 0;
    pthread_mutex_t done_yet = { 0 };
    pthread_cond_t done_cond = { 0 };
    ASSERT_EQ(0, pthread_mutex_init(&done_yet, NULL));
    ASSERT_EQ(0, pthread_mutex_lock(&done_yet));
    ASSERT_EQ(0, pthread_cond_init(&done_cond, NULL));
    // mutex will be unlocked once counter has finished counting

    pool = tpoolrr_create(TPOOLRR_TEST_FUNCTION3_THREADS, TPOOLRR_TEST_FUNCTION3_JOBS);
    ASSERT_NE(nullptr, pool);
    ASSERT_EQ(0, tpoolrr_handler_update(pool, tpoolrr_test_handler3));
    struct tpoolrr_test_arg3 arg = { &counter, &done_yet, &done_cond };
    ASSERT_EQ(0, tpoolrr_jobs_assign(pool, user_tag++, tpoolrr_test_worker3, (void *) &arg, 0));

    ASSERT_EQ(0, pthread_cond_wait(&done_cond, &done_yet));
    ASSERT_EQ(0, pthread_mutex_unlock(&done_yet));
    MT_LOG("JOINING");
    tpoolrr_join(pool);
    MT_LOG("JOINED");
    MT_LOG("counter: %i", counter);
    ASSERT_EQ(TPOOLRR_TEST_FUNCTION3_THREADS, counter);
    tpoolrr_destroy(pool);
}
}
