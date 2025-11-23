#include <tpollrr.h>
#include <tpollrr_priv.h>
#include <pointerarith.h>

uint64_t monotonic_time_now() {
    struct timespec spec;
    uint64_t seconds, milliseconds;

    clock_gettime(CLOCK_MONOTONIC, &spec);
    seconds = spec.tv_sec;
    milliseconds = spec.tv_nsec / 1000000;

    return (seconds * 1000) + milliseconds;
}

struct tpoolrr_job_construct(void (*function)(void*), void *arg, size_t expiration) {
    tpoolrr_job return_this;

    return_this.function = function;
    return_this.arg = arg;

    // check if we should set an expiration
    if (expiration > 0) {
        return_this.expiration = monotonic_time_now() + expiration;
    } else {
        return_this.expiration = 0;
    }

    return return_this;
}

void *consumer_loop_function(void *void_arg) {
    struct consumer_arg *arg;
    Aqueue *queue;
    sem_t *semaphore;
    tpoolrr_job job;
    int res;

    if(void_arg == NULL) {
        return NULL;
    }
    arg = (struct consumer_arg*) void_arg;

    queue = arg->job_queue;
    semaphore = arg->semaphore;
    while(true) {
        sem_wait(semaphore);

        res = aqueue_dequeue(queue, &job);
        if(res != 0) {
            // maybe log???;
        }

        if(job.expiration == 0) {
            // no expiration attached
        } else if(monotonic_time_now() < job.expiration) {
            job.function(job.arg);
        } else {
            // expired
        }

        // use pthread_wait to check if we have been paused
    }

    sem_destroy(semaphore);
    aqueue_deinit(queue);

    return NULL;
}

// Creates a thread pool with thread_count threads in which each thread can have jobs_per_thread max pending jobs
Tpoolrr *tpoolrr_create(size_t thread_count, size_t jobs_per_thread) {
    Tpoolrr *pool;

    pool = calloc(1, tpoolrr_advise(thread_count, jobs_per_thread));
    if(pool == NULL) {
        return NULL;
    }

    return pool;
}

// Advise how much memory a tpoolrr with thread_count threads each with jobs_per_thread max pending jobs needs
// Assumes that the same number of threads and jobs are requested when calling tpoolrr_advise and tpoolrr_init
size_t *tpoolrr_advise(size_t thread_count, size_t jobs_per_thread) {
    return (thread_count * sizeof(pthread_t)) +
           (thread_count * sizeof(aqueue));
}

// Uses pre-allocated memory greater-than-or-equal to what tpoolrr_advise returns when given same thread and job values
int tpoolrr_init(void *memory, size_t thread_count, size_t jobs_per_thread) {
    void *next_unused_region;
    if(memory == NULL) {
        return 1;
    }

    next_unused_region = memory
                         next_unused_region += 0;
    pool = next_unused_region;
    next_unused_region += 1 * sizeof(Tpoolrr);
    pool->threads = next_unused_region;
    next_unused_region += thread_count * sizeof(Tpoolrr);
    pool->job_queues = next_unused_region;
    pool->rr_index = 0;
    pool->threads_total = thread_count;
    pool->jobs_per_thread = jobs_per_thread;

    return pool;
}

// Deinitializes provided thread pool
// If the pool still has active jobs tpoolrr_stop_safe is called
// Passing a NULL pool causes nothing to happen
void tpoolrr_deinit(Tpoolrr *pool) {
    if(pool == NULL) {
        return;
    }

    tpoolrr_stop_safe(pool);

    return;
}

// This function should only be used if the pointer was returned by tpoolrr_create
// If the pool still has active jobs tpoolrr_stop_safe is called
// Passing a NULL pool causes nothing to happen
void tpoolrr_destroy(Tpoolrr *pool) {
    if(pool == NULL) {
        return;
    }

    tpoolrr_deinit(pool);
    free(pool);

    return;
}

// Add one job to the thread_pool
// When expiration is 0 then it is not enforced that this job should start before some time in the future
// When expiration is not 0 then this job is allowed to run only if less than expiration milliseconds have passed
int tpoolrr_add_one(Tpoolrr *pool, void (*function)(void*), void *arg, uint64_t expiration);

// Add many jobs to the thread_pool
// When expiration is 0 then it is not enforced that any of the jobs should start before some time in the future
// When expiration is not 0 then each job added is allowed to run only if less than expiration milliseconds have passed
// Some among the jobs added may start if they begin before expiration milliseconds have passed while while others die
int tpoolrr_add_many(Tpoolrr *pool, void ((**functions)(void*)), void **args, uint64_t expiration);

// Allow threads to finish their current jobs but pause running any future jobs until tpoolrr_resume is called
int tpoolrr_pause(Tpoolrr *pool);

// Allow threads to begin running queued jobs
int tpoolrr_resume(Tpoolrr *pool);

// Allow threads to finish their current jobs, however, destroy all queued jobs
int tpoolrr_stop_safe(Tpoolrr *pool);

// This function is dangerous!
// Stop current jobs as soon as possible and destroy all queued jobs
// The effect of stopping jobs while they are being done is by its nature unpredictable and likely to cause errors
int tpoolrr_stop_unsafe(Tpoolrr *pool);

// Wait for thread pool to finish all jobs
void tpoolrr_join(Tpoolrr *pool);

// Get number of active threads
// This is by its nature an O(N) operation
size_t tpoolrr_get_threads_active(Tpoolrr *pool);

// Get number of waiting threads
// This is by its nature an O(N) operation
size_t tpoolrr_get_threads_waiting(Tpoolrr *pool);

// Get number of threads in pool
size_t tpoolrr_get_threads_total(Tpoolrr *pool) {
    if(pool == NULL) {
        return 0;
    }

    return pool->threads_total;
}

// Get number of jobs the thread pool has in its current backlog
// This is by its nature an O(N) operation
size_t tpoolrr_get_jobs_queued(Tpoolrr *pool);

// Get number of jobs the thread pool can accept with its current backlog
// This is by its nature an O(N) operation
size_t tpoolrr_get_jobs_empty(Tpoolrr *pool);

// Get number of jobs the thread pool can accept assuming no jobs are currently queued
// This is by its nature an O(N) operation
size_t tpoolrr_get_jobs_cap(Tpoolrr *pool) {
    if(pool == NULL) {
        return 0;
    }

    return pool->threads_total * pool->jobs_per_thread;
}
