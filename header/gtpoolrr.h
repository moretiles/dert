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
    GTPOOLRR_THREAD_STATE_ACTIVE = 0,
    GTPOOLRR_THREAD_STATE_PAUSED = 1,
    GTPOOLRR_THREAD_STATE_JOINED = 2,
    GTPOOLRR_THREAD_STATE_STOPPED = 3,
    GTPOOLRR_THREAD_STATE_CANCELLED = 4
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

    // Used to allocate jobs used for submissions and completions
    Vpool *jobs;

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

    // Index used to control the next thread a job should be pushed to
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
// Init expects memory to be aligned to a multiple of 8
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

// Get number of submitted jobs the thread pool has in its current backlog
// This is by its nature an O(N) operation
size_t gtpoolrr_sbs_queued(Gtpoolrr *pool);

// Get number of submitted jobs the thread pool can accept with its current backlog
// This is by its nature an O(N) operation
size_t gtpoolrr_sbs_empty(Gtpoolrr *pool);

// Get number of submitted jobs the thread pool can accept assuming no jobs are currently queued
// This is by its nature an O(N) operation
size_t gtpoolrr_sbs_cap(Gtpoolrr *pool);

// Get job pointers that can then be filled in with data and submitted
int gtpoolrr_sbs_get(
    Gtpoolrr *thread_pool, struct gtpoolrr_job *jobs_dest[],
    size_t *num_obtained, size_t num_requested
);

// Set user tag for a single job
void gtpoolrr_sbs_set_tag(struct gtpoolrr_job *job, uint64_t user_tag);

// Set function for a single job
void gtpoolrr_sbs_set_function(
    struct gtpoolrr_job *job, 
    void *((*function)(volatile Gtpoolrr*, volatile Greent*, volatile void*))
);

// Set arg for a single job
void gtpoolrr_sbs_set_arg(struct gtpoolrr_job *job, void *arg);

// Set expiration to (current time now in nanoseconds + nanoseconds_from_now) for a single job
void gtpoolrr_sbs_set_expiration(struct gtpoolrr_job *job, uint64_t nanoseconds_from_now);

// Set user tag for num_jobs_to_set_for jobs
void gtpoolrr_sbs_set_tags(struct gtpoolrr_job *jobs[], size_t num_jobs_to_set_for, uint64_t user_tag);

// Set functions for num_jobs_to_set_for jobs
void gtpoolrr_sbs_set_functions(
    struct gtpoolrr_job *jobs[], size_t num_jobs_to_set_for,
    void *((*function)(volatile Gtpoolrr*, volatile Greent*, volatile void*))
);

// Set args for num_jobs_to_set_for jobs
void gtpoolrr_sbs_set_args(struct gtpoolrr_job *jobs[], size_t num_jobs_to_set_for, void *arg);

// Set expiration to (current time now in nanoseconds + nanoseconds_from_now) for num_jobs_to_set_for jobs
void gtpoolrr_sbs_set_expirations(
    struct gtpoolrr_job *jobs[], size_t num_jobs_to_set_for, uint64_t nanoseconds_from_now
);

// Add job to thread pool assigning to first available thread
int gtpoolrr_sbs_push(Gtpoolrr *pool, struct gtpoolrr_job *job);

// Add many jobs to thread pool assigning each to the first available thread
int gtpoolrr_sbs_pushall( Gtpoolrr *pool, size_t *num_pushed, size_t num_to_push, struct gtpoolrr_job *jobs[]);

// Add job to thread pool assigning to first available thread
int gtpoolrr_sbs_push_rr(Gtpoolrr *pool, struct gtpoolrr_job *job);

// Add many jobs to thread pool assigning each to the first available thread
int gtpoolrr_sbs_pushall_rr(Gtpoolrr *pool, size_t *num_pushed, size_t num_to_push, struct gtpoolrr_job *jobs[]);

// Add job for the thread specified by thread_index in the thread pool
int gtpoolrr_sbs_push_direct(Gtpoolrr *pool, size_t thread_index, struct gtpoolrr_job *job);

// Add many jobs for the thread specified by thread_index in the thread pool
int gtpoolrr_sbs_pushall_direct(
    Gtpoolrr *pool, size_t thread_index, size_t *num_pushed, size_t num_to_push,
    struct gtpoolrr_job *jobs[]
);

// Get number of completion events resulting from completed jobs for the thread pool
// This is by its nature an O(N) operation
size_t gtpoolrr_cps_queued(Gtpoolrr *pool);

// Get number of completion events that can still be accepted from completed jobs for the thread pool
// This is by its nature an O(N) operation
size_t gtpoolrr_cps_empty(Gtpoolrr *pool);

// Get number of completion events that the thread pool can accept assuming no completion events are queued
// This is by its nature an O(N) operation
size_t gtpoolrr_cps_cap(Gtpoolrr *pool);

// Get one completed job back
// Returns EBUSY if no submitted jobs
int gtpoolrr_cps_pop(Gtpoolrr *pool, struct gtpoolrr_job **dest);

// Get at most at_most completed jobs back
// Returns EBUSY if not able to meet at_most
// actual number obtained placed in num_popped
int gtpoolrr_cps_popsome(Gtpoolrr *pool, struct gtpoolrr_job *dest[], size_t *num_popped, size_t at_most);

// Block until getting wait_for completed job back
int gtpoolrr_cps_popall(Gtpoolrr *pool, struct gtpoolrr_job *dest[], size_t wait_for);

// Acknowledge several jobs
// These jobs should have been returned by completions_pop after submitting them
void gtpoolrr_cps_ack(Gtpoolrr *thread_pool, struct gtpoolrr_job *done_jobs[], size_t num_acknowledged);
