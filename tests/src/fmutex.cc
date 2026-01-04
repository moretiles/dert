namespace {
struct fmutex_test_worker_arg {
    Fmutex *mutex;
    int counter;
};

void *fmutex_test_worker(void *varg) {
    struct fmutex_test_worker_arg *arg;

    arg = (struct fmutex_test_worker_arg *) varg;
    fmutex_lock(arg->mutex);
    arg->counter += 1;
    fmutex_unlock(arg->mutex);

    return NULL;
}

TEST(fmutex, simple) {
    void *retval;
    struct fmutex_test_worker_arg arg;
    pthread_t threads[200];

    Fmutex *mutex = fmutex_create();
    ASSERT_NE(mutex, nullptr);

    arg.mutex = mutex;
    arg.counter = 0;
    for(size_t i = 0; i < 200; i++) {
        pthread_create(&threads[i], NULL, fmutex_test_worker, &arg);
    }

    for(size_t i = 0; i < 200; i++) {
        pthread_join(threads[i], &retval);
    }

    fmutex_destroy(mutex);
}
}
