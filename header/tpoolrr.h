#include <aqueue.h>

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

/*
 * tpoolrr.h -- Thread pool in which workers are served tasks using a round-robin task schedule
 * This thread pool is implemented using pthreads and aims to support Unix-based operating systems.
 * You are given the option of allowing this library to allocate a thread pool for you using tpoolrr_create.
 * Or, you can call tpoolrr_advise to get the memory total needed, allocate it, and call tpoolrr_init with that memory.
 * Because of these memory conventions tpoolrr_destroy should only be used with memory allocated by tpoolrr_create.
 * For the same reason, tpoolrr_deinit should only be used on memory initialized with tpoolrr_init.
 *
 * This library assumes a single producer, multiple consumer architecture.
 * A main thread creates the pool, assigns work, and ultimately destroys the pool.
 * Consumer threads are thus controlled only by the main thread as they perform work.
 * Because of these decisions, it is not safe for multiple threads/processes to act on the same Tpoolrr in parallel.
 * We hope you understand.
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

#ifndef TPOOLRR_LET_UNRECOVERABLE_ERRORS_FAIL_SILENTLY
#define TPOOLRR_LET_UNRECOVERABLE_ERRORS_FAIL_SILENTLY (0)
#else
#warning "The Tpoolrr thread pool will not try to exit the process when faced with unrecoverable errors"
#endif

#define PTHREAD_MUTEX_LOCK_UNLOCK_DESTROY_ATTEMPTS (10)
#define PTHREAD_COND_WAIT_SIGNAL_DESTROY_ATTEMPTS (10)
#define PTHREAD_CREATE_JOIN_ATTEMPTS (10)

// ACTIVE state means the thread should perform assigned jobs and expect new jobs
// PAUSED state means the thread should pause until a signal is sent to the condtion variable
// JOINED state means the thread should perform all its current assigned jobs and then call pthread_join once done
// STOPPED state means the thread should call pthread_join as soon as its current job is done
// CANCELLED state means the thread has been cancelled through pthread_cancel
enum tpoolrr_thread_state {
    TPOOLRR_THREAD_STATE_ACTIVE,
    TPOOLRR_THREAD_STATE_PAUSED,
    TPOOLRR_THREAD_STATE_JOINED,
    TPOOLRR_THREAD_STATE_STOPPED,
    TPOOLRR_THREAD_STATE_CANCELLED
};
// Round-Robin Thread Pool
typedef struct tpoolrr {
    // The pthreads created when pthread_create/pthread_init is called
    pthread_t *threads;

    // Worker arguments
    struct tpoolrr_worker_arg *worker_args;

    // Job queues, one for each thread
    Aqueue *job_queues;

    // The desired state that the associated thread should be in
    // Should only be accessed when the condition mutex is accquired
    enum tpoolrr_thread_state *desired_states;

    // The current state the thread has set
    // Should only be accessed when the condition mutex is accquired
    enum tpoolrr_thread_state *current_states;

    // Condition variable used to perform pausing for all threads
    pthread_cond_t *conditions;

    // Mutex used to control the condition variable associated with pausing all threads
    pthread_mutex_t *condition_mutexes;

    // Handler function that worker thread jobs can call to supervise
    void ((*handler_function)(struct tpoolrr *pool, void *arg));

    // Index used to control the next thread a job should be added to
    size_t rr_index;

    // Number of total threads, set when creating/initializing
    size_t threads_total;

    // Number of jobs each thread can have queued, set when creating/initializing
    size_t jobs_per_thread;
} Tpoolrr;

typedef void *((*Tpoolrr_fn) (void *));

// Creates a thread pool with thread_count threads in which each thread can have jobs_per_thread max pending jobs
Tpoolrr *tpoolrr_create(size_t thread_count, size_t jobs_per_thread);

// Advise how much memory a tpoolrr with thread_count threads each with jobs_per_thread max pending jobs needs
// Assumes that the same number of threads and jobs are requested when calling tpoolrr_advise and tpoolrr_init
size_t tpoolrr_advise(size_t thread_count, size_t jobs_per_thread);

// Uses pre-allocated memory greater-than-or-equal to what tpoolrr_advise returns when given same thread and job values
int tpoolrr_init(Tpoolrr **dest, void *memory, size_t thread_count, size_t jobs_per_thread);

// Deinitializes provided thread pool
// If the pool still has active jobs tpoolrr_stop_safe is called
// Passing a NULL pool causes nothing to happen
void tpoolrr_deinit(Tpoolrr *pool);

// This function should only be used if the pointer was returned by tpoolrr_create
// If the pool still has active jobs tpoolrr_stop_safe is called
// Passing a NULL pool causes nothing to happen
void tpoolrr_destroy(Tpoolrr *pool);

// Update handler associated with pool
// By default no handler is used
//int tpoolrr_handler_update(Tpoolrr *pool, void ((*function)(struct tpoolrr *pool, void *arg)));

// Call handler associated with pool
// By default no handler is used
//int tpoolrr_handler_call(Tpoolrr *pool);

// Returns 0 when handler is called or no jobs
// Returns with error if handler not set
//int tpoolrr_handler_wait(Tpoolrr *pool);

// Add one job to the thread_pool
// When freshness is 0 then it is not enforced that this job should start before some time in the future
// When freshness is not 0 then this job is allowed to run only if less than expiration milliseconds have passed
int tpoolrr_jobs_add(Tpoolrr *pool, void *((*function)(void*)), void *arg, size_t expiration);

// Add many jobs to the thread_pool
// When freshness is not 0 then each job added is allowed to run only if less than expiration milliseconds have passed
// Some among the jobs added may start if they begin before expiration milliseconds have passed while while others die
int tpoolrr_jobs_addall(Tpoolrr *pool, size_t count, void *((*functions[])(void*)), void *args[], size_t expiration);

// Add job to every single thread
// Fails with EBUSY if every thread does not have at least one free job space in its queue
// Does not add any jobs until it is confirmed that all threads have at least one free job space in their queue
int tpoolrr_jobs_assign(Tpoolrr *pool, void *((*function)(void*)), void *arg, size_t expiration);

// Allow threads to finish their current jobs but pause running any future jobs until tpoolrr_resume is called
int tpoolrr_pause(Tpoolrr *pool);

// Allow threads to begin running queued jobs
int tpoolrr_resume(Tpoolrr *pool);

// Allow threads to finish their current jobs, however, destroy all queued jobs
// Threads are then joined
int tpoolrr_stop_safe(Tpoolrr *pool);

// This function is dangerous!
// Stop current jobs as soon as possible and destroy all queued jobs
// Threads are cancelled
// The effect of stopping jobs while they are being done is by its nature unpredictable and likely to cause errors
int tpoolrr_stop_unsafe(Tpoolrr *pool);

// Wait for thread pool to finish all jobs
int tpoolrr_join(Tpoolrr *pool);

// Get number of active threads
// This is by its nature an O(N) operation
size_t tpoolrr_threads_active(Tpoolrr *pool);

// Get number of inactive threads
// This is by its nature an O(N) operation
size_t tpoolrr_threads_inactive(Tpoolrr *pool);

// Get number of threads in pool
size_t tpoolrr_threads_total(Tpoolrr *pool);

// Get number of jobs the thread pool has in its current backlog
// This is by its nature an O(N) operation
size_t tpoolrr_jobs_queued(Tpoolrr *pool);

// Get number of jobs the thread pool can accept with its current backlog
// This is by its nature an O(N) operation
size_t tpoolrr_jobs_empty(Tpoolrr *pool);

// Get number of jobs the thread pool can accept assuming no jobs are currently queued
// This is by its nature an O(N) operation
size_t tpoolrr_jobs_cap(Tpoolrr *pool);
