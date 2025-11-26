// needed for usleep
#define _XOPEN_SOURCE 500

// needed for pthread_timedjoin_np
#define _GNU_SOURCE 1

#include <tpoolrr.h>
#include <tpoolrr_priv.h>
#include <aqueue.h>
#include <pointerarith.h>
#include <stdlib.h>

#include <stddef.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

// pthread_mutex_init error policy
//
// EBUSY = Relevant. Tried to initalize mutex currently in-use
// EAGAIN
// ENOMEM
// EPERM = Not relevant. Caller does not have permission
// EINVAL

// pthread_mutex_lock error policy
//
// EAGAIN
// EINVAL = Not relevant. Possible when using PTHREEAD_PRIO_PROTECT
// ENOTRECOVERABLE
// EOWNERDEAD = Not relevant. Possible when using multi-processing
// EDEADLK
// EPERM = Not relevant. Possible when using error-checking, recursive, or robust mutexes

// pthread_mutex_unlock error policy
//
// EPERM = Not relevant. Current thread does not own mutex

// pthread_mutex_destroy error policy
//
// EBUSY = Relevant.
// EINVAL = Relevant.
// EAGAIN = Relevant.
// ENOMEM = Relevant.
// EPERM = Not relevant.

// pthread_cond_init
//
// EBUSY = Relevant.
// EINVAL = Relevant.
// EAGAIN = Relevant.
// ENOMEM = Relevant.

// pthread_cond_wait error policy
//
// EINVAL = Relevant. The pointed to mutex/condition are invalid or different mutexes were used in wait and signal
// EPERM = Not relevant. Mutex owned by different thread at end of call

// pthread_cond_signal error policy
//
// EINVAL = Relevant. Condition is not an initialized condition

// pthread_cond_destroy error policy
//
// EBUSY = Relevant. Already referenced elsewhere
// EINVAL

// pthread_create error policy
//
// EAGAIN
// EINVAL
// EPERM = Relevant. Unable to set scheduling parameters requested by attr

// pthread_cancel
//
// ESRCH = Relevant. Could not find thread (never created or already exited)

// pthread_join
//
// EDEADLK = Relevant. Deadlock is non-recoverable
// EINVAL = Relevant. Thread is either invalid or already being joined
// ESRCH = Relevant. Could not find thread (never created or already exited)

int try_pthread_mutex_lock(pthread_mutex_t *mutex) {
    int res;

    for(size_t i = 0; i < PTHREAD_MUTEX_LOCK_UNLOCK_DESTROY_ATTEMPTS; i++) {
        res = pthread_mutex_lock(mutex);
        if(res == 0) {
            return res;
        }

        // Wait one millisecond, hope no error next iteration
        usleep(1000);
    }

    return res;
}

void must_pthread_mutex_lock(pthread_mutex_t *mutex) {
#if TPOOLRR_LET_UNRECOVERABLE_ERRORS_FAIL_SILENTLY
    try_pthread_mutex_lock(mutex);
#else
    assert(try_pthread_mutex_lock(mutex) == 0);
#endif
}

int try_pthread_mutex_unlock(pthread_mutex_t *mutex) {
    int res;

    for(size_t i = 0; i < PTHREAD_MUTEX_LOCK_UNLOCK_DESTROY_ATTEMPTS; i++) {
        res = pthread_mutex_unlock(mutex);
        if(res == 0) {
            return res;
        }

        // Wait one millisecond, hope no error next iteration
        usleep(1000);
    }

    return res;
}

void must_pthread_mutex_unlock(pthread_mutex_t *mutex) {
#if TPOOLRR_LET_UNRECOVERABLE_ERRORS_FAIL_SILENTLY
    try_pthread_mutex_unlock(mutex);
#else
    assert(try_pthread_mutex_unlock(mutex) == 0);
#endif
}

int try_pthread_mutex_destroy(pthread_mutex_t *mutex) {
    int res;

    for(size_t i = 0; i < PTHREAD_MUTEX_LOCK_UNLOCK_DESTROY_ATTEMPTS; i++) {
        res = pthread_mutex_destroy(mutex);
        if(res == 0) {
            return res;
        }

        // Wait one millisecond, hope no error next iteration
        usleep(1000);
    }

    return res;
}

void must_pthread_mutex_destroy(pthread_mutex_t *mutex) {
#if TPOOLRR_LET_UNRECOVERABLE_ERRORS_FAIL_SILENTLY
    try_pthread_mutex_destroy(mutex);
#else
    assert(try_pthread_mutex_destroy(mutex) == 0);
#endif
}

int try_pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
    int res;

    for(size_t i = 0; i < PTHREAD_COND_WAIT_SIGNAL_DESTROY_ATTEMPTS; i++) {
        res = pthread_cond_wait(cond, mutex);
        if(res == 0) {
            return res;
        }

        // Wait one millisecond, hope no error next iteration
        usleep(1000);
    }

    return res;
}

void must_pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
#if TPOOLRR_LET_UNRECOVERABLE_ERRORS_FAIL_SILENTLY
    try_pthread_cond_wait(cond, mutex);
#else
    assert(try_pthread_cond_wait(cond, mutex) == 0);
#endif
}

int try_pthread_cond_signal(pthread_cond_t *cond) {
    int res;

    for(size_t i = 0; i < PTHREAD_COND_WAIT_SIGNAL_DESTROY_ATTEMPTS; i++) {
        res = pthread_cond_signal(cond);
        if(res == 0) {
            return res;
        }

        // Wait one millisecond, hope no error next iteration
        usleep(1000);
    }

    return res;
}

void must_pthread_cond_signal(pthread_cond_t *cond) {
#if TPOOLRR_LET_UNRECOVERABLE_ERRORS_FAIL_SILENTLY
    try_pthread_cond_signal(cond);
#else
    assert(try_pthread_cond_signal(cond) == 0);
#endif
}

int try_pthread_cond_destroy(pthread_cond_t *cond) {
    int res;

    for(size_t i = 0; i < PTHREAD_COND_WAIT_SIGNAL_DESTROY_ATTEMPTS; i++) {
        res = pthread_cond_destroy(cond);
        if(res == 0) {
            return res;
        }

        // Wait one millisecond, hope no error next iteration
        usleep(1000);
    }

    return res;
}

void must_pthread_cond_destroy(pthread_cond_t *cond) {
#if TPOOLRR_LET_UNRECOVERABLE_ERRORS_FAIL_SILENTLY
    try_pthread_cond_destroy(cond);
#else
    assert(try_pthread_cond_destroy(cond) == 0);
#endif
}

int try_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                       void *((*start_routine) (void*)), void *arg) {
    int res;

    for(size_t i = 0; i < PTHREAD_CREATE_JOIN_ATTEMPTS; i++) {
        res = pthread_create(thread, attr, start_routine, arg);
        if(res == 0) {
            return res;
        }

        // Wait one millisecond, hope no error next iteration
        usleep(1000);
    }

    return res;
}

void must_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                         void *((*start_routine) (void*)), void *arg) {
#if TPOOLRR_LET_UNRECOVERABLE_ERRORS_FAIL_SILENTLY
    try_pthread_create(thread, attr, start_routine, arg);
#else
    assert(try_pthread_create(thread, attr, start_routine, arg) == 0);
#endif
}

int try_pthread_join(Tpoolrr *pool, size_t index, void **retval) {
    int res;
    struct timespec t = { 0 };

    for(size_t i = 0; i < PTHREAD_CREATE_JOIN_ATTEMPTS; i++) {
        if(pool->current_states[index] == TPOOLRR_THREAD_STATE_JOINED) {
            return 0;
        }
        assert(clock_getres(CLOCK_REALTIME, &t) == 0);
        t.tv_sec = 0;
        t.tv_nsec += 10000000;
        res = try_pthread_mutex_lock(&(pool->condition_mutexes[index]));
        if(res != 0) {
            return res;
        }
        res = pthread_timedjoin_np(pool->threads[index], retval, &t);
        if(res == 0) {
            return 0;
        }
        res = try_pthread_cond_signal(&(pool->conditions[index]));
        if (res != 0) {
            return res;
        }
        res = try_pthread_mutex_unlock(&(pool->condition_mutexes[index]));
        if(res != 0) {
            return res;
        }
        sleep(1);
    }

    return res;
}

void must_pthread_join(Tpoolrr *pool, size_t index, void **retval) {
#if TPOOLRR_LET_UNRECOVERABLE_ERRORS_FAIL_SILENTLY
    try_pthread_join(pool, index, retval);
#else
    assert(try_pthread_join(pool, index, retval) == 0);
#endif
}

uint64_t monotonic_time_now() {
    struct timespec spec;
    uint64_t seconds, milliseconds;

    clock_gettime(CLOCK_MONOTONIC, &spec);
    seconds = spec.tv_sec;
    milliseconds = spec.tv_nsec / 1000000;

    return (seconds * 1000) + milliseconds;
}

struct tpoolrr_job tpoolrr_job_construct(void *(*function)(void*), void *arg, size_t expiration) {
    struct tpoolrr_job return_this;

    return_this.function = function;
    return_this.arg = arg;
    return_this.expiration = expiration;

    return return_this;
}

struct tpoolrr_worker_arg tpoolrr_worker_arg_construct(Tpoolrr *pool, size_t index) {
    struct tpoolrr_worker_arg return_this;

    return_this.pool = pool;
    return_this.index = index;

    return return_this;
}

void *tpoolrr_worker(void *void_arg) {
    struct tpoolrr_worker_arg *arg;
    Tpoolrr *pool;
    size_t index;
    Aqueue *job_queue;
    enum tpoolrr_thread_state *desired_state;
    enum tpoolrr_thread_state *current_state;
    pthread_cond_t *condition;
    pthread_mutex_t *condition_lock;

    struct tpoolrr_job job;
    int res;

    if(void_arg == NULL) {
        return NULL;
    }
    arg = (struct tpoolrr_worker_arg*) void_arg;

    pool = arg->pool;
    index = arg->index;
    job_queue = &(pool->job_queues[index]);
    desired_state = &(pool->desired_states[index]);
    current_state = &(pool->current_states[index]);
    condition = &(pool->conditions[index]);
    condition_lock = &(pool->condition_mutexes[index]);
    while(true) {
tpoolrr_worker_loop:
        //printf("%lu: trying to lock mutex!\n", index);
        pthread_mutex_lock(condition_lock);
        //printf("%lu: desired states is %i\n", index, (int) *desired_state);
        if(*desired_state == TPOOLRR_THREAD_STATE_ACTIVE) {
            // keep running jobs if they are available

            *current_state = TPOOLRR_THREAD_STATE_ACTIVE;

            if(aqueue_len(job_queue) != 0) {
                pthread_mutex_unlock(condition_lock);
            } else {
                // we want to let other threads run if this thread has no jobs
                *current_state = TPOOLRR_THREAD_STATE_PAUSED;
                //printf("%lu: waiting...\n", index);
                pthread_cond_wait(condition, condition_lock);
                pthread_mutex_unlock(condition_lock);
                //printf("%lu: awake\n", index);

                // need to handle new desired state so go back to beginning of loop
                goto tpoolrr_worker_loop;
            }
        } else if(*desired_state == TPOOLRR_THREAD_STATE_PAUSED) {
            // pause this thread

            *current_state = TPOOLRR_THREAD_STATE_PAUSED;
            pthread_cond_wait(condition, condition_lock);
            pthread_mutex_unlock(condition_lock);

            // need to handle new desired state so go back to beginning of loop
            goto tpoolrr_worker_loop;
        } else if(*desired_state == TPOOLRR_THREAD_STATE_JOINED) {
            // return once all work is processed

            if(aqueue_len(job_queue) == 0) {
                *current_state = TPOOLRR_THREAD_STATE_JOINED;
                pthread_mutex_unlock(condition_lock);
                return NULL;
            } else {
                *current_state = TPOOLRR_THREAD_STATE_ACTIVE;
                pthread_mutex_unlock(condition_lock);
            }
        } else if(*desired_state == TPOOLRR_THREAD_STATE_STOPPED) {
            // return leaving queued work not started

            *current_state = TPOOLRR_THREAD_STATE_STOPPED;
            pthread_mutex_unlock(condition_lock);
            return NULL;
        } else if(*desired_state == TPOOLRR_THREAD_STATE_CANCELLED) {
            // would expect cancel to stop this thread before return can be performed...
            // maybe if the operating system made threads uncancellable by default?

            *current_state = TPOOLRR_THREAD_STATE_CANCELLED;
            // try to unlock but don't enforce
            pthread_mutex_unlock(condition_lock);
            return NULL;
        } else {
            assert(1 == 0);
        }

        // get next job
        res = aqueue_dequeue(job_queue, &job);
        printf("%lu: Got job %p\n", index, (void *) job.arg);
        if(res != 0) {
            printf("Failed to dequeue!\n");
            // maybe log???;
        }

        // try to run next job
        if(job.expiration == 0) {
            // no expiration attached
            job.function(job.arg);
        } else if(monotonic_time_now() < job.expiration) {
            // before expiration time
            job.function(job.arg);
        } else {
            // expired
        }
    }

    printf("returning from thread\n");
    return NULL;
}

Tpoolrr *tpoolrr_create(size_t thread_count, size_t jobs_per_thread) {
    Tpoolrr *pool;
    void *memory;

    if(thread_count == 0 || jobs_per_thread == 0) {
        return NULL;
    }

    memory = calloc(1, tpoolrr_advise(thread_count, jobs_per_thread));
    if(memory == NULL) {
        return NULL;
    }

    if(tpoolrr_init(&pool, memory, thread_count, jobs_per_thread) != 0) {
        free(memory);
        memory = NULL;
        return NULL;
    }

    return pool;
}

size_t tpoolrr_advise(size_t thread_count, size_t jobs_per_thread) {
    return (1 * sizeof(Tpoolrr)) +
           (thread_count * sizeof(pthread_t)) +
           (thread_count * sizeof(struct tpoolrr_worker_arg)) +
           (thread_count * sizeof(Aqueue)) +
           (thread_count * sizeof(enum tpoolrr_thread_state)) +
           (thread_count * sizeof(enum tpoolrr_thread_state)) +
           (thread_count * sizeof(pthread_cond_t)) +
           (thread_count * sizeof(pthread_mutex_t));
}

int tpoolrr_init(Tpoolrr **dest, void *memory, size_t thread_count, size_t jobs_per_thread) {
    void *next_unused_region;
    Tpoolrr *pool;
    int res;

    if(dest == NULL || memory == NULL || thread_count == 0 || jobs_per_thread == 0) {
        return EINVAL;
    }

    next_unused_region = memory;
    next_unused_region = pointer_literal_addition(next_unused_region,
                         0);
    pool = next_unused_region;
    next_unused_region = pointer_literal_addition(next_unused_region,
                         1 * sizeof(Tpoolrr));
    pool->threads = next_unused_region;
    next_unused_region = pointer_literal_addition(next_unused_region,
                         thread_count * sizeof(pthread_t));
    pool->worker_args = next_unused_region;
    next_unused_region = pointer_literal_addition(next_unused_region,
                         thread_count * sizeof(struct tpoolrr_worker_arg));
    pool->job_queues = next_unused_region;
    next_unused_region = pointer_literal_addition(next_unused_region,
                         thread_count * sizeof(Aqueue));
    pool->desired_states = next_unused_region;
    next_unused_region = pointer_literal_addition(next_unused_region,
                         thread_count * sizeof(enum tpoolrr_thread_state));
    pool->current_states = next_unused_region;
    next_unused_region = pointer_literal_addition(next_unused_region,
                         thread_count * sizeof(enum tpoolrr_thread_state));
    pool->conditions = next_unused_region;
    next_unused_region = pointer_literal_addition(next_unused_region,
                         thread_count * sizeof(pthread_cond_t));
    pool->condition_mutexes = next_unused_region;
    pool->rr_index = 0;
    pool->threads_total = thread_count;
    pool->jobs_per_thread = jobs_per_thread;

    for(size_t i = 0; i < thread_count; i++) {
        aqueue_init(&(pool->job_queues[i]), sizeof(struct tpoolrr_job), jobs_per_thread);
        pool->desired_states[i] = TPOOLRR_THREAD_STATE_ACTIVE;
        pool->current_states[i] = TPOOLRR_THREAD_STATE_ACTIVE;
        res = pthread_cond_init(&(pool->conditions[i]), NULL);
        if(res != 0) {
            return res;
        }
        res = pthread_mutex_init(&(pool->condition_mutexes[i]), NULL);
        if(res != 0) {
            return res;
        }

        pool->worker_args[i] = tpoolrr_worker_arg_construct(pool, i);
        pthread_create(&(pool->threads[i]), NULL, tpoolrr_worker, (void *) &(pool->worker_args[i]));
    }

    *dest = pool;
    return 0;
}

void tpoolrr_deinit(Tpoolrr *pool) {
    if(pool == NULL) {
        return;
    }

#if TPOOLRR_LET_UNRECOVERABLE_ERRORS_FAIL_SILENTLY
    tpoolrr_stop_safe(pool);
#else
    assert(tpoolrr_stop_safe(pool) == 0);
#endif

    for(size_t i = 0; i < pool->threads_total; i++) {
        aqueue_deinit(&(pool->job_queues[i]));
        pthread_cond_destroy(&(pool->conditions[i]));
        pthread_mutex_destroy(&(pool->condition_mutexes[i]));
    }

    memset(pool, 0, sizeof(Tpoolrr));

    return;
}

void tpoolrr_destroy(Tpoolrr *pool) {
    if(pool == NULL) {
        return;
    }

    tpoolrr_deinit(pool);
    free(pool);

    return;
}

int tpoolrr_jobs_add(Tpoolrr *pool, void *(*function)(void*), void *arg, uint64_t expiration) {
    uint64_t expiration_as_monotonic_time;
    if(pool == NULL || function == NULL || arg == NULL) {
        return EINVAL;
    }

    if(expiration == 0) {
        expiration_as_monotonic_time = 0;
    } else {
        expiration_as_monotonic_time = monotonic_time_now() + expiration;
    }

    struct tpoolrr_job job = tpoolrr_job_construct(function, arg, expiration_as_monotonic_time);
    return _tpoolrr_jobs_add(pool, job);
}

int _tpoolrr_jobs_add(Tpoolrr *pool, struct tpoolrr_job job) {
    size_t i, index, len, cap;
    bool locked_mutex_from_here = false;
    int res;

    if(pool == NULL || job.function == NULL || job.arg == NULL) {
        return EINVAL;
    }

    for(i = 0; i <= pool->threads_total; i++) {
        index = (i + pool->rr_index) % pool->threads_total;
        len = aqueue_len(&(pool->job_queues[index]));
        cap = aqueue_cap(&(pool->job_queues[index]));
        if(len == cap) {
            // cannot push
            continue;
        }

        // thread may be waiting, send conditional signal
        res = aqueue_enqueue(&(pool->job_queues[index]), &job);
        if(res != 0) {
            goto _tpoolrr_jobs_add_error;
        }
        res = pthread_mutex_lock(&(pool->condition_mutexes[index]));
        if (res != 0) {
            goto _tpoolrr_jobs_add_error;
        }
        locked_mutex_from_here = true;
        res = pthread_cond_signal(&(pool->conditions[index]));
        if (res != 0) {
            goto _tpoolrr_jobs_add_error;
        }
        res = pthread_mutex_unlock(&(pool->condition_mutexes[index]));
        if (res != 0) {
            goto _tpoolrr_jobs_add_error;
        }
        locked_mutex_from_here = false;

        pool->rr_index += 1;
        return 0;
    }

    // tried to push to all threads and failed
    res = EBUSY;

_tpoolrr_jobs_add_error:
    if(locked_mutex_from_here) {
        pthread_mutex_unlock(&(pool->condition_mutexes[i]));
        locked_mutex_from_here = false;
    }
    return res;
}


int tpoolrr_jobs_addall(Tpoolrr *pool, size_t count,
                        void *((*functions[])(void*)), void *args[], uint64_t expiration) {
    int ret = 0;
    int res;
    uint64_t expiration_as_monotonic_time;
    struct tpoolrr_job job = { 0 };

    if(pool == NULL || functions == NULL || args == NULL || count == 0) {
        ret = EINVAL;
        goto tpoolrr_jobs_addall_end;
    }

    if(expiration == 0) {
        expiration_as_monotonic_time = 0;
    } else {
        expiration_as_monotonic_time = monotonic_time_now() + expiration;
    }

    for(size_t i = 0; i < count; i++) {
        for(size_t j = 0; j < 3; j++) {
            // need to use monotonic api rather than relative time so that all jobs have same end time
            job = tpoolrr_job_construct(functions[i], args[i], expiration_as_monotonic_time);
            res = _tpoolrr_jobs_add(pool, job);

            // sleeping introduces latency, however, better than spinning up one core to 100% when there is high load
            if(res == 0) {
                break;
            } else if(res == EBUSY) {
                usleep(1000);
                continue;
            }
        }

        if(res != 0) {
            ret = res;
            goto tpoolrr_jobs_addall_end;
        }
    }

tpoolrr_jobs_addall_end:
    return ret;
}

int tpoolrr_jobs_assign(Tpoolrr *pool, void *((*function)(void*)), void *arg, size_t expiration) {
    int ret = 0;
    int res = 0;
    uint64_t expiration_as_monotonic_time;
    struct tpoolrr_job job = { 0 };

    if(pool == NULL || function == NULL || arg == NULL) {
        ret = EINVAL;
        goto tpoolrr_jobs_assign_end;
    }

    if(expiration == 0) {
        expiration_as_monotonic_time = 0;
    } else {
        expiration_as_monotonic_time = monotonic_time_now() + expiration;
    }

    // need to create job here to use so that tpoolrr_jobs_add can be called with jobs that all have the same end time
    job = tpoolrr_job_construct(function, arg, expiration_as_monotonic_time);

    // Make sure there is space for one additional job on every thread or fail with EBUSY
    for(size_t i = 0; i < pool->threads_total; i++) {
        if(tpoolrr_jobs_empty(pool) == 0) {
            return EBUSY;
        }
    }

    // Enqueue job
    for(size_t i = 0; i < pool->threads_total; i++) {
        // _tpoolrr_jobs_add increments rr_index by 1 if it succeeds in enqueueing a job
        // As it is confirmed in the prior loop that a job can be added to each thread this never fails with EBUSY
        res = _tpoolrr_jobs_add(pool, job);

        // only reason failure is critical threading problem
#if TPOOLRR_LET_UNRECOVERABLE_ERRORS_FAIL_SILENTLY
        res = res;
#else
        assert(res == 0);
#endif
    }

tpoolrr_jobs_assign_end:
    return ret;
}

int tpoolrr_pause(Tpoolrr *pool) {
    int res;
    size_t i = 0;
    bool locked_mutex_from_here = false;

    if(pool == NULL) {
        return EINVAL;
    }

    for(i = 0; i < pool->threads_total; i++) {
        res = pthread_mutex_lock(&(pool->condition_mutexes[i]));
        if(res != 0) {
            goto tpoolrr_pause_error;
        }
        locked_mutex_from_here = true;
        pool->desired_states[i] = TPOOLRR_THREAD_STATE_PAUSED;
        res = pthread_cond_signal(&(pool->conditions[i]));
        if (res != 0) {
            goto tpoolrr_pause_error;
        }
        res = pthread_mutex_unlock(&(pool->condition_mutexes[i]));
        if (res != 0) {
            goto tpoolrr_pause_error;
        }
        locked_mutex_from_here = false;
    }

tpoolrr_pause_error:
    if(locked_mutex_from_here) {
        pthread_mutex_unlock(&(pool->condition_mutexes[i]));
        locked_mutex_from_here = false;
    }
    return res;
}

int tpoolrr_resume(Tpoolrr *pool) {
    int res;
    size_t i = 0;
    bool locked_mutex_from_here = false;

    if(pool == NULL) {
        return EINVAL;
    }

    for(i = 0; i < pool->threads_total; i++) {
        res = pthread_mutex_lock(&(pool->condition_mutexes[i]));
        if(res != 0) {
            goto tpoolrr_resume_error;
        }
        locked_mutex_from_here = true;
        pool->desired_states[i] = TPOOLRR_THREAD_STATE_ACTIVE;
        res = pthread_cond_signal(&(pool->conditions[i]));
        if (res != 0) {
            goto tpoolrr_resume_error;
        }
        res = pthread_mutex_unlock(&(pool->condition_mutexes[i]));
        if (res != 0) {
            goto tpoolrr_resume_error;
        }
        locked_mutex_from_here = false;
    }

tpoolrr_resume_error:
    if(locked_mutex_from_here) {
        pthread_mutex_unlock(&(pool->condition_mutexes[i]));
        locked_mutex_from_here = false;
    }
    return res;
}

int tpoolrr_stop_safe(Tpoolrr *pool) {
    int res = 0;
    void *thread_return_value;
    size_t i = 0;
    bool locked_mutex_from_here = false;

    if(pool == NULL) {
        return EINVAL;
    }

    for(i = 0; i < pool->threads_total; i++) {
        res = pthread_mutex_lock(&(pool->condition_mutexes[i]));
        if(res != 0) {
            goto tpoolrr_stop_safe_error;
        }
        locked_mutex_from_here = true;
        pool->desired_states[i] = TPOOLRR_THREAD_STATE_STOPPED;
        res = pthread_cond_signal(&(pool->conditions[i]));
        if (res != 0) {
            goto tpoolrr_stop_safe_error;
        }
        res = pthread_mutex_unlock(&(pool->condition_mutexes[i]));
        if (res != 0) {
            goto tpoolrr_stop_safe_error;
        }
        locked_mutex_from_here = false;
    }

    for(i = 0; i < pool->threads_total; i++) {
        if(pool->current_states[i] != TPOOLRR_THREAD_STATE_JOINED &&
                pool->current_states[i] != TPOOLRR_THREAD_STATE_CANCELLED) {
            res = pthread_join(pool->threads[i], &thread_return_value) == 0;
            if(res != 0) {
                goto tpoolrr_stop_safe_error;
            }
        }
    }

tpoolrr_stop_safe_error:
    if(locked_mutex_from_here) {
        pthread_mutex_unlock(&(pool->condition_mutexes[i]));
        locked_mutex_from_here = false;
    }
    return res;
}

int tpoolrr_stop_unsafe(Tpoolrr *pool) {
    if(pool == NULL) {
        return EINVAL;
    }

    for(size_t i = 0; i < pool->threads_total; i++) {
        // don't try to accquire mutex because stop_unsafe may be called to handle deadlock/timeout
        pool->desired_states[i] = TPOOLRR_THREAD_STATE_CANCELLED;
        pool->current_states[i] = TPOOLRR_THREAD_STATE_CANCELLED;

        // don't check error, just attempt
        pthread_cancel(pool->threads[i]);
    }

    // threads can disable cancellation and deadlock/timeout may prevent timely responses
    // best to just return as success
    // make it clear that this is not a clean way to end active jobs
    return 0;
}

int tpoolrr_join(Tpoolrr *pool) {
    void *thread_return_value;
    int res = 0;
    size_t i = 0;
    bool locked_mutex_from_here = false;
    bool thread_already_joined = false;
    bool thread_already_cancelled = false;
    bool need_to_join_this_thread = false;

    if(pool == NULL) {
        return EINVAL;
    }

    for(i = 0; i < pool->threads_total; i++) {
        res = pthread_mutex_lock(&(pool->condition_mutexes[i]));
        if(res != 0) {
            goto tpoolrr_join_error;
        }
        locked_mutex_from_here = true;

        pool->desired_states[i] = TPOOLRR_THREAD_STATE_JOINED;
        thread_already_joined = pool->current_states[i] == TPOOLRR_THREAD_STATE_JOINED;
        thread_already_cancelled = pool->current_states[i] == TPOOLRR_THREAD_STATE_CANCELLED;
        need_to_join_this_thread = !thread_already_joined || !thread_already_cancelled;

        res = pthread_mutex_unlock(&(pool->condition_mutexes[i]));
        if(res != 0) {
            goto tpoolrr_join_error;
        }
        locked_mutex_from_here = false;

        if(need_to_join_this_thread) {
            res = pthread_cond_signal(&(pool->conditions[i]));
            if(res != 0) {
                goto tpoolrr_join_error;
            }
            res = pthread_join(pool->threads[i], &thread_return_value);
            if(res != 0) {
                goto tpoolrr_join_error;
            }
        }
    }

tpoolrr_join_error:
    if(locked_mutex_from_here) {
        pthread_mutex_unlock(&(pool->condition_mutexes[i]));
        locked_mutex_from_here = false;
    }
    return res;
}

size_t tpoolrr_threads_active(Tpoolrr *pool) {
    size_t total;
    int res;

    if (pool == NULL) {
        return 0;
    }

    for(size_t i = 0; i < pool->threads_total; i++) {
        res = pthread_mutex_lock(&(pool->condition_mutexes[i]));
        if(res != 0) {
            return 0;
        }
        if (pool->current_states[i] == TPOOLRR_THREAD_STATE_ACTIVE) {
            total++;
        }
        res = pthread_mutex_unlock(&(pool->condition_mutexes[i]));
        if(res != 0) {
            return 0;
        }
    }

    return 0;
}

size_t tpoolrr_threads_inactive(Tpoolrr *pool) {
    int res;
    size_t total;

    if (pool == NULL) {
        return 0;
    }

    for(size_t i = 0; i < pool->threads_total; i++) {
        res = pthread_mutex_lock(&(pool->condition_mutexes[i]));
        if(res != 0) {
            return 0;
        }
        if (pool->current_states[i] != TPOOLRR_THREAD_STATE_ACTIVE) {
            total++;
        }
        res = pthread_mutex_unlock(&(pool->condition_mutexes[i]));
        if(res != 0) {
            return 0;
        }
    }

    return 0;
}

size_t tpoolrr_threads_total(Tpoolrr *pool) {
    if(pool == NULL) {
        return 0;
    }

    return pool->threads_total;
}

size_t tpoolrr_jobs_queued(Tpoolrr *pool) {
    size_t total;

    if(pool == NULL) {
        return 0;
    }

    for(size_t i = 0; i < pool->threads_total; i++) {
        total += aqueue_len(&(pool->job_queues[i]));
    }

    return total;
}

size_t tpoolrr_jobs_empty(Tpoolrr *pool) {
    size_t total;

    if(pool == NULL) {
        return 0;
    }

    for(size_t i = 0; i < pool->threads_total; i++) {
        total += aqueue_cap(&(pool->job_queues[i])) - aqueue_len(&(pool->job_queues[i]));
    }

    return total;
}

size_t tpoolrr_jobs_cap(Tpoolrr *pool) {
    if(pool == NULL) {
        return 0;
    }

    return pool->threads_total * pool->jobs_per_thread;
}
