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
 * Because of these decisions, it is not safe for multiple threads/processes to act on the same Tpoolrr at the same time.
 * We hope you understand.
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

// Round-Robin Thread Pool
typedef struct tpoolrr {
    // The pthreads created when pthread_create/pthread_init is called
    pthread_t *threads;

    // Job queues, one for each thread
    aqueue *job_queues;

    // Used for waking and sleeping threads
    sem_t *semaphores;

    // Condition variable used to perform pausing for all threads
    pthread_cond_t condition;

    // Mutex used to control the condition variable associated with pausing all threads
    pthread_mutex_t condition_mutex;

    // Index used to control the next thread a job should be added to
    size_t rr_index;

    // Number of total threads, set when creating/initializing
    size_t threads_total;

    // Number of jobs each thread can have queued, set when creating/initializing
    size_t jobs_per_thread;
} Tpoolrr;

// Creates a thread pool with thread_count threads in which each thread can have jobs_per_thread max pending jobs
Tpoolrr *tpoolrr_create(size_t thread_count, size_t jobs_per_thread);

// Advise how much memory a tpoolrr with thread_count threads each with jobs_per_thread max pending jobs needs
// Assumes that the same number of threads and jobs are requested when calling tpoolrr_advise and tpoolrr_init
size_t *tpoolrr_advise(size_t thread_count, size_t jobs_per_thread);

// Uses pre-allocated memory greater-than-or-equal to what tpoolrr_advise returns when given same thread and job values
int tpoolrr_init(void *memory, size_t thread_count, size_t jobs_per_thread);

// Deinitializes provided thread pool
// If the pool still has active jobs tpoolrr_stop_safe is called
// Passing a NULL pool causes nothing to happen
void tpoolrr_deinit(Tpoolrr *pool);

// This function should only be used if the pointer was returned by tpoolrr_create
// If the pool still has active jobs tpoolrr_stop_safe is called
// Passing a NULL pool causes nothing to happen
void tpoolrr_destroy(Tpoolrr *pool);

// Add one job to the thread_pool
// When freshness is 0 then it is not enforced that this job should start before some time in the future
// When freshness is not 0 then this job is allowed to run only if less than expiration milliseconds have passed
int tpoolrr_add_one(Tpoolrr *pool, void (*function)(void*), void *arg, size_t expiration);

// Add many jobs to the thread_pool
// When freshness is not 0 then each job added is allowed to run only if less than expiration milliseconds have passed
// Some among the jobs added may start if they begin before expiration milliseconds have passed while while others die
int tpoolrr_add_many(Tpoolrr *pool, void ((**functions)(void*)), void **args, size_t expiration);

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
size_t tpoolrr_get_threads_total(Tpoolrr *pool);

// Get number of jobs the thread pool has in its current backlog
// This is by its nature an O(N) operation
size_t tpoolrr_get_jobs_queued(Tpoolrr *pool);

// Get number of jobs the thread pool can accept with its current backlog
// This is by its nature an O(N) operation
size_t tpoolrr_get_jobs_empty(Tpoolrr *pool);

// Get number of jobs the thread pool can accept assuming no jobs are currently queued
// This is by its nature an O(N) operation
size_t tpoolrr_get_jobs_cap(Tpoolrr *pool);
