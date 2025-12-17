/*
 * gtpoolrr.h -- Green thread pool in which workers are served tasks using a round-robin task schedule
 * This thread pool is implemented using pthreads and aims to support Unix-based operating systems.
 * You are given the option of allowing this library to allocate a thread pool for you using gtpoolrr_create.
 * Or, you can call gtpoolrr_advise to get the memory total needed, allocate it, and call gtpoolrr_init with that memory.
 * Because of these memory conventions gtpoolrr_destroy should only be used with memory allocated by gtpoolrr_create.
 * For the same reason, gtpoolrr_deinit should only be used on memory initialized with gtpoolrr_init.
 *
 * This library assumes a single producer, multiple consumer architecture.
 * A main thread creates the pool, assigns work, and ultimately destroys the pool.
 * Consumer threads are thus controlled only by the main thread as they perform work.
 * Because of these decisions, it is not safe for multiple threads/processes to act on the same Gtpoolrr in parallel.
 * We hope you understand.
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

#pragma once

#include <vpool.h>
#include <vqueue.h>
#include <aqueue.h>
#include <greent.h>

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

#ifndef GTPOOLRR_LET_UNRECOVERABLE_ERRORS_FAIL_SILENTLY
#define GTPOOLRR_LET_UNRECOVERABLE_ERRORS_FAIL_SILENTLY (0)
#else
#warning "The Gtpoolrr thread pool will not try to exit the process when faced with unrecoverable errors"
#endif

#define PTHREAD_MUTEX_LOCK_UNLOCK_DESTROY_ATTEMPTS (10)
#define PTHREAD_COND_WAIT_SIGNAL_DESTROY_ATTEMPTS (10)
#define PTHREAD_CREATE_JOIN_ATTEMPTS (10)

// ACTIVE state means the thread should perform assigned jobs and expect new jobs
// PAUSED state means the thread should pause until a signal is sent to the condtion variable
// JOINED state means the thread should perform all its current assigned jobs and then call pthread_join once done
// STOPPED state means the thread should call pthread_join as soon as its current job is done
// CANCELLED state means the thread has been cancelled through pthread_cancel
enum gtpoolrr_thread_state {
    GTPOOLRR_THREAD_STATE_ACTIVE,
    GTPOOLRR_THREAD_STATE_PAUSED,
    GTPOOLRR_THREAD_STATE_JOINED,
    GTPOOLRR_THREAD_STATE_STOPPED,
    GTPOOLRR_THREAD_STATE_CANCELLED
};

// Strategy to use when assigning work to threads
// ROUNDROBIN strategy means that work is distributed in round robin order
// HASHED strategy means that each piece of work is distributed by hashing a key
enum gtpoolrr_thread_strategy {
    GTPOOLRR_THREAD_STRATEGY_ROUNDROBIN,
    GTPOOLRR_THREAD_STRATEGY_HASHED
};

// Number of type of threads that should be created
// NOTHREAD backend means creating worker threads and starting them with a main function is left to end-user
// ONETHREAD backend means one worker thread is created and handles incoming work across all channels
// MANYTHREADS backend means num_threads worker threads are created and each handles incoming work on one channel
// ONEGREENTHREAD backend means one worker thread is created and is used to spawn green threads
// MANYGREENTHREADS backend means num_threads worker threads are created and each spawns green threads
enum gtpoolrr_thread_backend {
    GTPOOLRR_THREAD_BACKEND_NOTHREAD,
    GTPOOLRR_THREAD_BACKEND_ONETHREAD,
    GTPOOLRR_THREAD_BACKEND_MANYTHREADS,
    GTPOOLRR_THREAD_BACKEND_ONEGREENTHREAD,
    GTPOOLRR_THREAD_BACKEND_MANYGREENTHREADS
};

// Round-Robin Thread Pool
typedef struct gtpoolrr {
    // check if already stopped
    // needed to prevent accessing threads when using stop_unsafe;
    bool stopped;

    // The pthreads created when pthread_create/pthread_init is called
    pthread_t *threads;

    // The green threads used by the threads themselves
    Vpool **green_threads;

    // Used by each thread to map from unique_id to the green thread pointer itself
    Greent **thread_map;

    // Used by each thread to track its ready threads
    Vqueue *ready_threads;

    // Worker arguments
    struct gtpoolrr_worker_arg *worker_args;

    // Submission queues, one for each thread
    Aqueue *job_submission_queues;

    // Completion queues, one for each thread
    Aqueue *job_completion_queues;

    // The desired state that the associated thread should be in
    // Applies to threads themselves, not green threads
    _Atomic enum gtpoolrr_thread_state *desired_states;

    // The current state the thread has set
    // Applies to threads themselves, not green threads
    _Atomic enum gtpoolrr_thread_state *current_states;

    // Handler function that worker thread jobs can call to supervise
    void *((*handler_function)(volatile struct gtpoolrr *pool, volatile Greent *thread, volatile void *arg));

    // Index used to control the next thread a job should be added to
    size_t rr_index;

    // Number of total threads, set when creating/initializing
    size_t threads_total;

    // Number of jobs each thread can have queued, set when creating/initializing
    size_t jobs_per_thread;
} Gtpoolrr;


// Represents a single job that should be performed by one of the threads
struct gtpoolrr_job {
    // unique tag to use
    // cosmetic
    uint64_t user_tag;

    // used during submission
    struct {
        void *((*function)(volatile Gtpoolrr*, volatile Greent*, volatile void*));
        void *arg;
        uint64_t expiration;
    };

    // used during completion
    struct {
        size_t thread_assigned_to;
        void *ret;
        void *flags;
    };
};

typedef void *((*Gtpoolrr_fn) (volatile Gtpoolrr *, volatile Greent *, volatile void *));

// Creates a thread pool with thread_count threads in which each thread can have jobs_per_thread max pending jobs
Gtpoolrr *gtpoolrr_create(size_t thread_count, size_t jobs_per_thread);

// Advise how much memory a gtpoolrr with thread_count threads each with jobs_per_thread max pending jobs needs
// Assumes that the same number of threads and jobs are requested when calling gtpoolrr_advise and gtpoolrr_init
size_t gtpoolrr_advise(size_t thread_count, size_t jobs_per_thread);

// Uses pre-allocated memory greater-than-or-equal to what gtpoolrr_advise returns when given same thread and job values
int gtpoolrr_init(Gtpoolrr **dest, void *memory, size_t thread_count, size_t jobs_per_thread);

// Deinitializes provided thread pool
// If the pool still has active jobs gtpoolrr_stop_safe is called
// Passing a NULL pool causes nothing to happen
void gtpoolrr_deinit(Gtpoolrr *pool);

// This function should only be used if the pointer was returned by gtpoolrr_create
// If the pool still has active jobs gtpoolrr_stop_safe is called
// Passing a NULL pool causes nothing to happen
void gtpoolrr_destroy(Gtpoolrr *pool);

// Allow threads to finish their current jobs but pause running any future jobs until gtpoolrr_resume is called
int gtpoolrr_pause(Gtpoolrr *pool);

// Allow threads to begin running queued jobs
int gtpoolrr_resume(Gtpoolrr *pool);

// Allow threads to finish their current jobs, however, destroy all queued jobs
// Threads are then joined
int gtpoolrr_stop_safe(Gtpoolrr *pool);

// This function is dangerous!
// Stop current jobs as soon as possible and destroy all queued jobs
// Threads are cancelled
// The effect of stopping jobs while they are being done is by its nature unpredictable and likely to cause errors
int gtpoolrr_stop_unsafe(Gtpoolrr *pool);

// Wait for thread pool to finish all jobs
int gtpoolrr_join(Gtpoolrr *pool);

// Update handler associated with pool
// By default no handler is used
int gtpoolrr_handler_update(Gtpoolrr *pool, void *((*function)(volatile Gtpoolrr*, volatile Greent*, volatile void*)));

// Call handler associated with pool
// By default no handler is used
int gtpoolrr_handler_call(Gtpoolrr *pool, Greent *green_thread, void *arg, void **retval);

// Add one job to the thread_pool
// When freshness is 0 then it is not enforced that this job should start before some time in the future
// When freshness is not 0 then this job is allowed to run only if less than expiration milliseconds have passed
int gtpoolrr_jobs_add(Gtpoolrr *pool, uint64_t user_tag, void *((*function)(volatile Gtpoolrr*, volatile Greent*, volatile void*)), void *arg, size_t expiration);

// Add many jobs to the thread_pool
// When freshness is not 0 then each job added is allowed to run only if less than expiration milliseconds have passed
// Some among the jobs added may start if they begin before expiration milliseconds have passed while while others die
int gtpoolrr_jobs_addall(Gtpoolrr *pool, size_t count, uint64_t user_tag[], void *((*functions[])(volatile Gtpoolrr*, volatile Greent*, volatile void*)), void *args[], size_t expiration);

// Add job to every single thread
// Fails with EBUSY if every thread does not have at least one free job space in its queue
// Does not add any jobs until it is confirmed that all threads have at least one free job space in their queue
int gtpoolrr_jobs_assign(Gtpoolrr *pool, uint64_t user_tag, void *((*function)(volatile Gtpoolrr*, volatile Greent*, volatile void*)), void *arg, size_t expiration);

// Get one completed job back
// Returns EBUSY if no submitted jobs
int gtpoolrr_completions_pop(Gtpoolrr *pool, struct gtpoolrr_job *dest);

// Get at most at_most completed jobs back
// Returns EBUSY if not able to meet at_most
// actual number obtained placed in num_popped
int gtpoolrr_completions_popsome(Gtpoolrr *pool, struct gtpoolrr_job dest[], size_t at_most, size_t *num_popped);

// Block until getting wait_for completed job back
int gtpoolrr_completions_popall(Gtpoolrr *pool, struct gtpoolrr_job dest[], size_t wait_for);

// Get number of submitted jobs the thread pool has in its current backlog
// This is by its nature an O(N) operation
size_t gtpoolrr_submissions_queued(Gtpoolrr *pool);

// Get number of submitted jobs the thread pool can accept with its current backlog
// This is by its nature an O(N) operation
size_t gtpoolrr_submissions_empty(Gtpoolrr *pool);

// Get number of submitted jobs the thread pool can accept assuming no jobs are currently queued
// This is by its nature an O(N) operation
size_t gtpoolrr_submissions_cap(Gtpoolrr *pool);

// Get number of completion events resulting from completed jobs for the thread pool
// This is by its nature an O(N) operation
size_t gtpoolrr_completions_queued(Gtpoolrr *pool);

// Get number of completion events that can still be accepted from completed jobs for the thread pool
// This is by its nature an O(N) operation
size_t gtpoolrr_completions_empty(Gtpoolrr *pool);

// Get number of completion events that the thread pool can accept assuming no completion events are queued
// This is by its nature an O(N) operation
size_t gtpoolrr_completions_cap(Gtpoolrr *pool);
