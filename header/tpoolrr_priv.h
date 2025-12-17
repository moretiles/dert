/*
 * tpoolrr_priv.h -- Thread pool in which workers are served tasks using a round-robin task schedule
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

#pragma once

// The type used as an argument to tpoolrr_worker
struct tpoolrr_worker_arg {
    Tpoolrr *pool;
    size_t index;
};

// Monotonic time in milliseconds
uint64_t monotonic_time_now(void);

// Returns job construct assembled from arguments
struct tpoolrr_job tpoolrr_job_construct(
        uint64_t user_tag, void *((*start_routine) (Tpoolrr*,void*)),
        void *arg, size_t expiration
    );

struct tpoolrr_worker_arg tpoolrr_worker_arg_construct(Tpoolrr *pool, size_t index);

// Function spawned on new thread when initalizing/creating the thread pool
void *tpoolrr_worker(void *void_arg);

int _tpoolrr_jobs_add(Tpoolrr *pool, struct tpoolrr_job job);
