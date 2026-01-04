namespace {
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
    arg = (struct fsemaphore_test_function_arg *) varg;

    assert(fsemaphore_wait(arg->sem) == 0);
    __atomic_fetch_add(&(arg->array[arg->index % FSEMAPHORE_SEM_MAX]), arg->index, __ATOMIC_ACQ_REL);
    usleep(100);
    if(arg->do_post) {
        assert(fsemaphore_post(arg->sem) == 0);
    }

    return NULL;
}

TEST(fsemaphore, simple) {
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

}
}
