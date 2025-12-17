// needed for usleep
#define _XOPEN_SOURCE 500

// needed for pthread_timedjoin_np
#define _GNU_SOURCE 1

#include <gtpoolrr.h>
#include <gtpoolrr_priv.h>
#include <vpool.h>
#include <aqueue.h>
#include <vqueue.h>
#include <greent.h>
#include <pointerarith.h>

#include <stdlib.h>
#include <stddef.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

uint64_t gt_monotonic_time_now(void) {
    struct timespec spec;
    uint64_t seconds, milliseconds;

    clock_gettime(CLOCK_MONOTONIC, &spec);
    seconds = spec.tv_sec;
    milliseconds = spec.tv_nsec / 1000000;

    return (seconds * 1000) + milliseconds;
}

struct gtpoolrr_job gtpoolrr_job_construct(
    uint64_t user_tag, void *(*function)(volatile Gtpoolrr*, volatile Greent*, volatile void*),
    void *arg, size_t expiration
) {
    struct gtpoolrr_job return_this;

    return_this.user_tag = user_tag;
    return_this.function = function;
    return_this.arg = arg;
    return_this.expiration = expiration;

    return return_this;
}

struct gtpoolrr_worker_arg gtpoolrr_worker_arg_construct(Gtpoolrr *pool, size_t index) {
    struct gtpoolrr_worker_arg return_this;

    return_this.pool = pool;
    return_this.index = index;

    return return_this;
}

struct gtpoolrr_worker_desired_and_current_state {
    _Atomic enum gtpoolrr_thread_state *desired_state;
    _Atomic enum gtpoolrr_thread_state *current_state;
};

void gtpoolrr_worker_destructor_current(void *varg) {
    if(varg == NULL) {
        return;
    }

    struct gtpoolrr_worker_desired_and_current_state *desired_and_current_state = varg;
    *(desired_and_current_state->current_state) = *(desired_and_current_state->desired_state);

    return;
}

void *gtpoolrr_worker(void *void_arg) {
    volatile struct gtpoolrr_worker_arg *arg;
    volatile Greent *running_thread;

    struct io_uring_cqe *cqe;
    struct io_uring ring;
    Vert vert;
    struct gtpoolrr_job job;
    int res;
    size_t counter = 0;

    if(void_arg == NULL) {
        return NULL;
    }
    arg = (struct gtpoolrr_worker_arg*) void_arg;

    volatile Gtpoolrr *const pool = arg->pool;
    const volatile size_t index = arg->index;
    volatile Vpool *const green_threads = pointer_literal_addition(pool->green_threads, index * vpool_advise(pool->jobs_per_thread, greent_advise()));
    volatile Greent **const thread_map = pointer_literal_addition(pool->thread_map, index * pool->jobs_per_thread * sizeof(Greent *));
    volatile Vqueue *const ready_threads = pointer_literal_addition(pool->ready_threads, index * vqueue_advise(pool->jobs_per_thread, sizeof(Greent*)));
    volatile Aqueue *const submission_queue = &(pool->job_submission_queues[index]);
    volatile Aqueue *const completion_queue = &(pool->job_completion_queues[index]);
    _Atomic volatile enum gtpoolrr_thread_state *const desired_state = &(pool->desired_states[index]);
    _Atomic volatile enum gtpoolrr_thread_state *const current_state = &(pool->current_states[index]);

    const int io_uring_queue_init_res = io_uring_queue_init(pool->jobs_per_thread, &ring, 0);
    if(io_uring_queue_init_res < 0) {
        // lock mutex and set state to stopped
        return NULL;
    }

    running_thread = greent_root(&vert);
    if(running_thread != NULL) {
        size_t num_submissions = 0;
        greent_do_submit(running_thread, &ring, &num_submissions);
        if(num_submissions > 0) {
            thread_map[running_thread->unique_id] = running_thread;
            running_thread = NULL;
        }
    }

    volatile bool keep_going = true;
    while(keep_going) {
        unsigned i = 0;
        unsigned head;
        io_uring_for_each_cqe(&ring, head, cqe) {
            thread_map[cqe->user_data]->completion.user_data = cqe->user_data;
            thread_map[cqe->user_data]->completion.res = cqe->res;
            thread_map[cqe->user_data]->completion.flags = cqe->flags;

            if(vqueue_enqueue((Vqueue *) ready_threads, &(thread_map[cqe->user_data]), false) != 0) {
                assert(false);
            }
            i++;
        }
        if(i != 0) {
            io_uring_cq_advance(&ring, i);
        }

        // inlining the use of desired state because of some problems with _Atomic volatile enums
        //switch(*desired_state)
        switch(pool->desired_states[index]) {
        case GTPOOLRR_THREAD_STATE_ACTIVE:
            pool->current_states[index] = GTPOOLRR_THREAD_STATE_ACTIVE;
            // inlining the use of current state because of some problems with _Atomic volatile enums
            break;
        case GTPOOLRR_THREAD_STATE_PAUSED:
            // inlining the use of current state because of some problems with _Atomic volatile enums
            pool->current_states[index] = GTPOOLRR_THREAD_STATE_PAUSED;
            usleep(1000);
            continue;
            break;
        case GTPOOLRR_THREAD_STATE_JOINED:
            if(
                (aqueue_len((Aqueue *) submission_queue) == 0) &&
                (vqueue_len((Vqueue *) ready_threads) == 0)
            ) {
                // current state needs to be set by a pthread destructor
                keep_going = false;
                continue;
            }
            break;
        case GTPOOLRR_THREAD_STATE_STOPPED:
            // current state needs to be set by a pthread destructor
            keep_going = false;
            continue;
            break;
        case GTPOOLRR_THREAD_STATE_CANCELLED:
            // current state needs to be set by a pthread destructor
            keep_going = false;
            continue;
            break;
        }

        // Think about dealing with expiration
        if(running_thread == NULL) {
            res = vqueue_dequeue((Vqueue *) ready_threads, &running_thread);
            if(res != 0 && res != ENODATA) {
                assert(false);
            }
        }

        if(running_thread != NULL) {
            greent_resume(running_thread, 0);
        } else {
            printf("thread: %lu has aqueue with length of %lu!\n", index, aqueue_len((Aqueue *) submission_queue));
            if(aqueue_len((Aqueue *) submission_queue) == 0) {
                // temporary way of pausing
                usleep(1000);
                continue;
            }
            res = aqueue_dequeue((Aqueue *) submission_queue, &job);
            if(res != 0) {
                printf("Failed to dequeue!\n");
                // maybe log???;
            }
            printf("Got job %lu:%lu\n", index, (size_t) job.user_tag);
            job.thread_assigned_to = index;
            job.flags = 0;

            running_thread = vpool_alloc((Vpool *) green_threads);
            if(running_thread == NULL) {
                assert(false);
            } else if(running_thread->parent == NULL) {
                running_thread->parent = &vert;
                running_thread->unique_id = counter++;
            }

            job.function(pool, running_thread, job.arg);
            thread_map[running_thread->unique_id] = NULL;
            running_thread = NULL;
        }

        printf("Completed job %lu:%lu\n", index, (size_t) job.user_tag);
        res = aqueue_enqueue((Aqueue *) completion_queue, &job);
        if(res != 0) {
            printf("Failed to enqueue!\n");
            // maybe log???;
        }
    }

    // free io_uring

    // stack frames no longer used for functions so scan store stuff
    pthread_key_t destructor_key;
    struct gtpoolrr_worker_desired_and_current_state *destructor_arg = (void*) pool->green_threads;
    // inlining both desired_state and current_state because of issues involving _Atomic volatile enum types
    destructor_arg->desired_state = &(pool->desired_states[index]);
    destructor_arg->current_state = &(pool->current_states[index]);
    pthread_key_create(&destructor_key, gtpoolrr_worker_destructor_current);
    pthread_setspecific(destructor_key, destructor_arg);

    printf("returning from thread\n");
    return NULL;
}

Gtpoolrr *gtpoolrr_create(size_t thread_count, size_t jobs_per_thread) {
    Gtpoolrr *pool;
    void *memory;

    if(thread_count == 0 || jobs_per_thread == 0) {
        return NULL;
    }

    memory = malloc(gtpoolrr_advise(thread_count, jobs_per_thread));
    if(memory == NULL) {
        return NULL;
    }

    if(gtpoolrr_init(&pool, memory, thread_count, jobs_per_thread) != 0) {
        free(memory);
        memory = NULL;
        return NULL;
    }

    return pool;
}

size_t gtpoolrr_advise(size_t thread_count, size_t jobs_per_thread) {

    return (1 * sizeof(Gtpoolrr)) +
           (thread_count * sizeof(pthread_t)) +
           (thread_count * vpool_advise(jobs_per_thread, greent_advise())) +
           (1 * greent_advisev(thread_count)) +
           (1 * vqueue_advisev(thread_count, jobs_per_thread, sizeof(Greent *))) +
           (thread_count * sizeof(struct gtpoolrr_worker_arg)) +
           (1 * aqueue_advisev(thread_count, sizeof(struct gtpoolrr_job), jobs_per_thread)) + // submissions
           (1 * aqueue_advisev(thread_count, sizeof(struct gtpoolrr_job), jobs_per_thread)) + // completions
           (thread_count * sizeof(_Atomic enum gtpoolrr_thread_state)) +
           (thread_count * sizeof(_Atomic enum gtpoolrr_thread_state));
}

int gtpoolrr_init(Gtpoolrr **dest, void *memory, size_t thread_count, size_t jobs_per_thread) {
    Gtpoolrr *pool;
    int res;

    if(dest == NULL || memory == NULL || thread_count == 0 || jobs_per_thread == 0) {
        return EINVAL;
    }

    // Find memory locations
    {
        void *ptr = memory;
        ptr = pointer_literal_addition(ptr, 0);
        pool = ptr;
        if(memset(pool, 0, gtpoolrr_advise(thread_count, jobs_per_thread)) != pool) {
            return ENOTRECOVERABLE;
        }

        ptr = pointer_literal_addition(ptr, 1 * sizeof(Gtpoolrr));
        pool->threads = ptr;
        ptr = pointer_literal_addition(ptr, thread_count * sizeof(pthread_t));
        pool->green_threads = ptr;
        ptr = pointer_literal_addition(ptr, thread_count * vpool_advise(jobs_per_thread, greent_advise()));
        pool->thread_map = ptr;
        ptr = pointer_literal_addition(ptr, thread_count * jobs_per_thread * sizeof(Greent*));
        pool->ready_threads = ptr;
        ptr = pointer_literal_addition(ptr, vqueue_advisev(thread_count, jobs_per_thread, sizeof(Greent*)));
        pool->worker_args = ptr;
        ptr = pointer_literal_addition(ptr, thread_count * sizeof(struct gtpoolrr_worker_arg));
        pool->job_submission_queues = ptr;
        ptr = pointer_literal_addition(ptr, 1 * aqueue_advisev(thread_count, sizeof(struct gtpoolrr_job), jobs_per_thread));
        pool->job_completion_queues = ptr;
        ptr = pointer_literal_addition(ptr, 1 * aqueue_advisev(thread_count, sizeof(struct gtpoolrr_job), jobs_per_thread));
        pool->desired_states = ptr;
        ptr = pointer_literal_addition(ptr, thread_count * sizeof(_Atomic enum gtpoolrr_thread_state));
        pool->current_states = ptr;
        ptr = pointer_literal_addition(ptr, thread_count * sizeof(_Atomic enum gtpoolrr_thread_state));
    }

    // Actually initialize
    {
        res = vqueue_initv(thread_count, &(pool->ready_threads), pool->ready_threads, sizeof(Greent*), jobs_per_thread);
        if(res != 0) {
            return res;
        }
        res = aqueue_initv(thread_count, &(pool->job_submission_queues), (void *) pool->job_submission_queues, sizeof(struct gtpoolrr_job), jobs_per_thread);
        if(res != 0) {
            return res;
        }
        res = aqueue_initv(thread_count, &(pool->job_completion_queues), (void *) pool->job_completion_queues, sizeof(struct gtpoolrr_job), jobs_per_thread);
        if(res != 0) {
            return res;
        }

        for(size_t i = 0; i < thread_count; i++) {
            Vpool *vpool_dest_addr = pointer_literal_addition(pool->green_threads, i * vpool_advise(jobs_per_thread, greent_advise()));
            vpool_init(&vpool_dest_addr, vpool_dest_addr, jobs_per_thread, greent_advise(), VPOOL_KIND_STATIC);
            pool->worker_args[i] = gtpoolrr_worker_arg_construct(pool, i);
            _Atomic enum gtpoolrr_thread_state *desired_state = pointer_literal_addition(pool->desired_states, i * sizeof(_Atomic enum gtpoolrr_thread_state));
            *desired_state = GTPOOLRR_THREAD_STATE_ACTIVE;
            _Atomic enum gtpoolrr_thread_state *current_state = pointer_literal_addition(pool->current_states, i * sizeof(_Atomic enum gtpoolrr_thread_state));
            *current_state = GTPOOLRR_THREAD_STATE_ACTIVE;
        }

        pool->stopped = false;
        pool->handler_function = NULL;
        pool->rr_index = 0;
        pool->threads_total = thread_count;
        pool->jobs_per_thread = jobs_per_thread;

        for(size_t i = 0; i < thread_count; i++) {
            pthread_create(&(pool->threads[i]), NULL, gtpoolrr_worker, (void *) &(pool->worker_args[i]));
        }
    }

    *dest = pool;
    return 0;
}

void gtpoolrr_deinit(Gtpoolrr *pool) {
    if(pool == NULL) {
        return;
    }

    gtpoolrr_stop_safe(pool);
    memset(pool, 0, sizeof(Gtpoolrr));

    return;
}

void gtpoolrr_destroy(Gtpoolrr *pool) {
    if(pool == NULL) {
        return;
    }

    gtpoolrr_deinit(pool);
    free(pool);

    return;
}

int gtpoolrr_jobs_add(Gtpoolrr *pool, uint64_t user_tag, void *(*function)(volatile Gtpoolrr*, volatile Greent *, volatile void*), void *arg, uint64_t expiration) {
    uint64_t expiration_as_monotonic_time;
    if(pool == NULL || function == NULL || arg == NULL) {
        return EINVAL;
    }

    if(expiration == 0) {
        expiration_as_monotonic_time = 0;
    } else {
        expiration_as_monotonic_time = gt_monotonic_time_now() + expiration;
    }

    struct gtpoolrr_job job = gtpoolrr_job_construct(user_tag, function, arg, expiration_as_monotonic_time);
    return _gtpoolrr_jobs_add(pool, job);
}

int _gtpoolrr_jobs_add(Gtpoolrr *pool, struct gtpoolrr_job job) {
    size_t i, index, len, cap;
    int res;

    if(pool == NULL || job.function == NULL || job.arg == NULL) {
        return EINVAL;
    }

    for(i = 0; i <= pool->threads_total; i++) {
        index = (i + pool->rr_index) % pool->threads_total;
        len = aqueue_len(&(pool->job_submission_queues[index]));
        cap = aqueue_cap(&(pool->job_submission_queues[index]));
        if(len == cap) {
            // cannot push
            continue;
        }

        res = aqueue_enqueue(&(pool->job_submission_queues[index]), &job);
        if(res != 0) {
            goto _gtpoolrr_jobs_add_error;
        }

        pool->rr_index += 1;
        return 0;
    }

    // tried to push to all threads and failed
    res = EBUSY;

_gtpoolrr_jobs_add_error:
    return res;
}

int gtpoolrr_jobs_addall(
    Gtpoolrr *pool, size_t count,
    uint64_t user_tags[], void *((*functions[])(volatile Gtpoolrr*, volatile Greent*, volatile void*)),
    void *args[], uint64_t expiration
) {
    int ret = 0;
    int res;
    uint64_t expiration_as_monotonic_time;
    struct gtpoolrr_job job = { 0 };

    if(pool == NULL || user_tags == NULL || functions == NULL || args == NULL || count == 0) {
        ret = EINVAL;
        goto gtpoolrr_jobs_addall_end;
    }

    if(expiration == 0) {
        expiration_as_monotonic_time = 0;
    } else {
        expiration_as_monotonic_time = gt_monotonic_time_now() + expiration;
    }

    for(size_t i = 0; i < count; i++) {
        for(size_t j = 0; j < 3; j++) {
            // need to use monotonic api rather than relative time so that all jobs have same end time
            job = gtpoolrr_job_construct(user_tags[i], functions[i], args[i], expiration_as_monotonic_time);
            res = _gtpoolrr_jobs_add(pool, job);

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
            goto gtpoolrr_jobs_addall_end;
        }
    }

gtpoolrr_jobs_addall_end:
    return ret;
}

int gtpoolrr_jobs_assign(Gtpoolrr *pool, uint64_t user_tag, void *((*function)(volatile Gtpoolrr*, volatile Greent*, volatile void*)), void *arg, size_t expiration) {
    int ret = 0;
    int res = 0;
    uint64_t expiration_as_monotonic_time;
    struct gtpoolrr_job job = { 0 };

    if(pool == NULL || function == NULL || arg == NULL) {
        ret = EINVAL;
        goto gtpoolrr_jobs_assign_end;
    }

    if(expiration == 0) {
        expiration_as_monotonic_time = 0;
    } else {
        expiration_as_monotonic_time = gt_monotonic_time_now() + expiration;
    }

    // need to create job here to use so that gtpoolrr_jobs_add can be called with jobs that all have the same end time
    job = gtpoolrr_job_construct(user_tag, function, arg, expiration_as_monotonic_time);

    // Make sure there is space for one additional job on every thread or fail with EBUSY
    for(size_t i = 0; i < pool->threads_total; i++) {
        if(gtpoolrr_submissions_empty(pool) == 0) {
            return EBUSY;
        }
    }

    // Enqueue job
    for(size_t i = 0; i < pool->threads_total; i++) {
        // _gtpoolrr_jobs_add increments rr_index by 1 if it succeeds in enqueueing a job
        // As it is confirmed in the prior loop that a job can be added to each thread this never fails with EBUSY
        res = _gtpoolrr_jobs_add(pool, job);

        // only reason failure is critical threading problem
#if GTPOOLRR_LET_UNRECOVERABLE_ERRORS_FAIL_SILENTLY
        res = res;
#else
        assert(res == 0);
#endif
    }

gtpoolrr_jobs_assign_end:
    return ret;
}

int gtpoolrr_pause(Gtpoolrr *pool) {
    if(pool == NULL) {
        return EINVAL;
    }

    for(size_t i = 0; i < pool->threads_total; i++) {
        pool->desired_states[i] = GTPOOLRR_THREAD_STATE_PAUSED;
    }

    return 0;
}

int gtpoolrr_resume(Gtpoolrr *pool) {
    if(pool == NULL) {
        return EINVAL;
    }

    for(size_t i = 0; i < pool->threads_total; i++) {
        pool->desired_states[i] = GTPOOLRR_THREAD_STATE_ACTIVE;
    }

    return 0;
}

int gtpoolrr_stop_safe(Gtpoolrr *pool) {
    if(pool == NULL) {
        return EINVAL;
    }

    size_t i = 0;
    enum gtpoolrr_thread_state current_state;
    while(i < pool->threads_total) {
        pool->desired_states[i] = GTPOOLRR_THREAD_STATE_STOPPED;
        current_state = pool->current_states[i];
        const bool current_state_already_stopped = (current_state == GTPOOLRR_THREAD_STATE_STOPPED);
        const bool current_state_already_cancelled = (current_state == GTPOOLRR_THREAD_STATE_CANCELLED);
        const bool current_state_already_joined = (current_state == GTPOOLRR_THREAD_STATE_JOINED);
        const bool thread_already_done = current_state_already_stopped ||
                                         current_state_already_cancelled ||
                                         current_state_already_joined;
        if(thread_already_done) {
            i++;
        } else {
            usleep(1000);
        }
    }

    pool->stopped = true;

    return 0;
}

int gtpoolrr_stop_unsafe(Gtpoolrr *pool) {
    if(pool == NULL) {
        return EINVAL;
    }

    for(size_t i = 0; i < pool->threads_total; i++) {
        pool->desired_states[i] = GTPOOLRR_THREAD_STATE_CANCELLED;
        pool->current_states[i] = GTPOOLRR_THREAD_STATE_CANCELLED;

        // don't check error, just attempt
        pthread_cancel(pool->threads[i]);
    }
    pool->stopped = true;

    // threads can disable cancellation and deadlock/timeout may prevent timely responses
    // best to just return as success
    // make it clear that this is not a clean way to end active jobs
    return 0;
}

int gtpoolrr_join(Gtpoolrr *pool) {
    if(pool == NULL) {
        return EINVAL;
    }

    size_t i = 0;
    enum gtpoolrr_thread_state current_state;
    while(i < pool->threads_total) {
        pool->desired_states[i] = GTPOOLRR_THREAD_STATE_JOINED;
        current_state = pool->current_states[i];
        const bool current_state_already_stopped = (current_state == GTPOOLRR_THREAD_STATE_STOPPED);
        const bool current_state_already_cancelled = (current_state == GTPOOLRR_THREAD_STATE_CANCELLED);
        const bool current_state_already_joined = (current_state == GTPOOLRR_THREAD_STATE_JOINED);
        const bool thread_already_done = current_state_already_stopped ||
                                         current_state_already_cancelled ||
                                         current_state_already_joined;
        if(thread_already_done) {
            i++;
        } else {
            usleep(1000);
        }
    }

    pool->stopped = true;

    return 0;
}

int gtpoolrr_handler_update(Gtpoolrr *pool, void *((*function)(volatile struct gtpoolrr*, volatile Greent*, volatile void*))) {
    if(pool == NULL || function == NULL) {
        return EINVAL;
    }

    pool->handler_function = function;

    return 0;
}

int gtpoolrr_handler_call(Gtpoolrr *pool, Greent *green_thread, void *arg, void **retval) {
    if(pool == NULL || pool->handler_function == NULL || retval == NULL) {
        return EINVAL;
    }

    *retval = pool->handler_function(
                  (volatile Gtpoolrr*) pool, (volatile Greent*) green_thread, (volatile void*) arg
              );

    return 0;
}

int gtpoolrr_completions_pop(Gtpoolrr *pool, struct gtpoolrr_job *dest) {
    struct gtpoolrr_job job;
    bool done = false;
    if(pool == NULL || dest == NULL) {
        return EINVAL;
    }

    size_t i;
    for(i = 0; i < pool->threads_total; i++) {
        size_t index = (i + pool->rr_index) % pool->threads_total;
        const int aqueue_dequeue_res = aqueue_dequeue(&(pool->job_completion_queues[index]), &job);
        if(aqueue_dequeue_res == 0) {
            // found a completed job
            if(memcpy(dest, &job, sizeof(struct gtpoolrr_job)) != dest) {
                return ENOTRECOVERABLE;
            }
            done = true;
            break;
        } else if(aqueue_dequeue_res == ENODATA) {
            // try next queue
        } else {
            // something weird
            return aqueue_dequeue_res;
        }
    }

    if(!done) {
        return EBUSY;
    } else {
        pool->rr_index = (pool->rr_index + i) % pool->threads_total;
        return 0;
    }
}

int gtpoolrr_completions_popsome(Gtpoolrr *pool, struct gtpoolrr_job dest[], size_t at_most, size_t *num_popped) {
    size_t index_into_completion_queues = 0, index_into_dest = 0;
    int res = 0;
    if(pool == NULL || dest == NULL || at_most == 0 || num_popped == NULL) {
        res = EINVAL;
        goto gtpoolrr_completions_popsome_end;
    }

    while(index_into_completion_queues < at_most) {
        const int aqueue_dequeue_res = aqueue_dequeue(
                                           &(pool->job_completion_queues[index_into_completion_queues]),
                                           &(dest[index_into_dest])
                                       );

        if(aqueue_dequeue_res == 0) {
            // pass
            index_into_completion_queues++;
            index_into_dest++;
        } else if(aqueue_dequeue_res == EBUSY) {
            index_into_completion_queues++;
        } else {
            res = aqueue_dequeue_res;
            goto gtpoolrr_completions_popsome_end;
        }
    }

gtpoolrr_completions_popsome_end:
    *num_popped = index_into_dest;
    return res;
}

int gtpoolrr_completions_popall(Gtpoolrr *pool, struct gtpoolrr_job dest[], size_t wait_for) {
    int res = 0;
    if(pool == NULL || dest == NULL || wait_for == 0) {
        res = EINVAL;
        goto gtpoolrr_completions_popall_end;
    }

    size_t i = 0;
    while(i < wait_for) {
        const int gtpoolrr_completions_pop_res = gtpoolrr_completions_pop(pool, &(dest[i]));

        if(gtpoolrr_completions_pop_res == 0) {
            // pass
            i++;
        } else if(gtpoolrr_completions_pop_res == EBUSY) {
            usleep(1000);
            continue;
        } else {
            res = gtpoolrr_completions_pop_res;
            goto gtpoolrr_completions_popall_end;
        }
    }

gtpoolrr_completions_popall_end:
    return res;
}

size_t gtpoolrr_submissions_queued(Gtpoolrr *pool) {
    size_t total = 0;

    if(pool == NULL) {
        return 0;
    }

    for(size_t i = 0; i < pool->threads_total; i++) {
        total += aqueue_len(&(pool->job_submission_queues[i]));
    }

    return total;
}

size_t gtpoolrr_submissions_empty(Gtpoolrr *pool) {
    size_t total = 0;

    if(pool == NULL) {
        return 0;
    }

    for(size_t i = 0; i < pool->threads_total; i++) {
        total += aqueue_cap(&(pool->job_submission_queues[i])) - aqueue_len(&(pool->job_submission_queues[i]));
    }

    return total;
}

size_t gtpoolrr_submissions_cap(Gtpoolrr *pool) {
    if(pool == NULL) {
        return 0;
    }

    return pool->threads_total * pool->jobs_per_thread;
}

size_t gtpoolrr_completions_queued(Gtpoolrr *pool) {
    size_t total = 0;

    if(pool == NULL) {
        return 0;
    }

    for(size_t i = 0; i < pool->threads_total; i++) {
        total += aqueue_len(&(pool->job_completion_queues[i]));
    }

    return total;
}

size_t gtpoolrr_completions_empty(Gtpoolrr *pool) {
    size_t total = 0;

    if(pool == NULL) {
        return 0;
    }

    for(size_t i = 0; i < pool->threads_total; i++) {
        total += aqueue_cap(&(pool->job_completion_queues[i])) - aqueue_len(&(pool->job_completion_queues[i]));
    }

    return total;
}

size_t gtpoolrr_completions_cap(Gtpoolrr *pool) {
    if(pool == NULL) {
        return 0;
    }

    return pool->threads_total * pool->jobs_per_thread;
}
