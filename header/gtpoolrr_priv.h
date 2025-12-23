/*
 * gtpoolrr_priv.h -- Thread pool in which green threads are served tasks using a round-robin task schedule
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

#include <stddef.h>
#include <stdint.h>

// The type used as an argument to gtpoolrr_worker
struct gtpoolrr_worker_arg {
    Gtpoolrr *pool;
    size_t index;
};

// Monotonic time in milliseconds
uint64_t gt_monotonic_time_now(void);

// Prepare arg for worker function
struct gtpoolrr_worker_arg gtpoolrr_worker_arg_construct(Gtpoolrr *pool, size_t index);

// Function spawned on new thread when initalizing/creating the thread pool
void *gtpoolrr_worker(void *void_arg);
