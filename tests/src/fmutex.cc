namespace {
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

TEST(fmutex, simple) {
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
}
}
