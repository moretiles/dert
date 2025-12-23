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

struct gtpoolrr_worker_arg gtpoolrr_worker_arg_construct(Gtpoolrr *pool, size_t index) {
    struct gtpoolrr_worker_arg return_this;

    return_this.pool = pool;
    return_this.index = index;

    return return_this;
}

struct gtpoolrr_worker_desired_and_current_state {
    _Atomic volatile enum gtpoolrr_thread_state *desired_state;
    _Atomic volatile enum gtpoolrr_thread_state *current_state;
};

struct gtpoolrr_job *gtpoolrr_job_run(
    Gtpoolrr *volatile thread_pool, volatile size_t thread_index, Greent *volatile green_thread,
    struct gtpoolrr_job *volatile job, void *volatile flags
) {
    if(job == NULL) {
        return NULL;
    }

    job->ret = job->function(thread_pool, green_thread, job->arg);

    job->thread_assigned_to = thread_index;
    job->flags = flags; // this does nothing for now

    return job;
}

void gtpoolrr_worker_destructor_current(void *varg) {
    if(varg == NULL) {
        return;
    }

    struct gtpoolrr_worker_desired_and_current_state *desired_and_current_state = varg;
    *(desired_and_current_state->current_state) = *(desired_and_current_state->desired_state);

    return;
}

void *gtpoolrr_worker(void *void_arg) {
    struct gtpoolrr_worker_arg *arg;
    Greent *volatile running_thread;

    struct io_uring_cqe *cqe;
    struct io_uring ring;
    Vert vert;
    struct gtpoolrr_job *job;
    int res;
    size_t counter = 0;

    if(void_arg == NULL) {
        return NULL;
    }
    arg = (struct gtpoolrr_worker_arg*) void_arg;

    Gtpoolrr *volatile const pool = arg->pool;
    const volatile size_t index = arg->index;
    Vpool *volatile const green_threads = pointer_literal_addition(pool->green_threads, index * vpool_advise(pool->jobs_per_thread, greent_advise()));
    Vqueue *volatile const ready_threads = pointer_literal_addition(pool->ready_threads, index * vqueue_advise(pool->jobs_per_thread, sizeof(Greent*)));
    Aqueue *volatile const submission_queue = &(pool->job_submission_queues[index]);
    Aqueue *volatile const completion_queue = &(pool->job_completion_queues[index]);
    _Atomic enum gtpoolrr_thread_state *volatile const desired_state = &(pool->desired_states[index]);
    _Atomic enum gtpoolrr_thread_state *volatile const current_state = &(pool->current_states[index]);

    const int io_uring_queue_init_res = io_uring_queue_init(pool->jobs_per_thread * 100, &ring, 0);
    if(io_uring_queue_init_res < 0) {
        // lock mutex and set state to stopped
        return NULL;
    }

    running_thread = greent_root(&vert);
    if(running_thread != NULL) {
        size_t num_submissions = 0;
        greent_do_submit((Greent *) running_thread, &ring, &num_submissions);
        if(num_submissions > 0) {
            running_thread = NULL;
        }
    }

    volatile bool keep_going = true;
    while(keep_going) {
        unsigned i = 0;
        unsigned head;
        io_uring_for_each_cqe(&ring, head, cqe) {
            Greent *waiting_thread;
            bool cqe_for_timeout, something, something_else;
            res = greent_unpack(cqe->user_data, &waiting_thread, &cqe_for_timeout, &something, &something_else);
            assert(res == 0);
            printf("%lu = Received CQE for operation %lu\n", waiting_thread->unique_id, waiting_thread->submission.do_this);
            if(cqe_for_timeout) {
                // do nothing for timeouts
                // the primary operation (read, write, etc.) will return with its own response
            } else {
                waiting_thread->completion.user_data = cqe->user_data;
                waiting_thread->completion.res = cqe->res;
                waiting_thread->completion.flags = cqe->flags;

                if(vqueue_enqueue((Vqueue *) ready_threads, &waiting_thread, false) != 0) {
                    assert(false);
                }
            }

            i++;
        }
        if(i != 0) {
            io_uring_cq_advance(&ring, i);
        }

        // inlining the use of desired state because of some problems with _Atomic volatile enums
        switch(*desired_state) {
        case GTPOOLRR_THREAD_STATE_ACTIVE:
            *current_state = GTPOOLRR_THREAD_STATE_ACTIVE;
            break;
        case GTPOOLRR_THREAD_STATE_PAUSED:
            *current_state = GTPOOLRR_THREAD_STATE_PAUSED;
            usleep(1000);
            continue;
            break;
        case GTPOOLRR_THREAD_STATE_JOINED:
            if(
                running_thread == NULL &&
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
            res = vqueue_dequeue((Vqueue *) ready_threads, (void *) &running_thread);
            if(res != 0 && res != ENODATA) {
                assert(false);
            }
        }

        if(running_thread != NULL) {
            greent_resume(running_thread, 0);
        } else {
            //printf("thread: %lu has aqueue with length of %lu!\n", index, aqueue_len((Aqueue *) submission_queue));
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
            printf("Got job %lu:%lu\n", index, (size_t) job->user_tag);
            job->thread_assigned_to = index;
            job->flags = 0;

            running_thread = vpool_alloc((Vpool *) green_threads);
            if(running_thread == NULL) {
                assert(false);
            } else if(running_thread->parent == NULL) {
                running_thread->parent = &vert;
                running_thread->unique_id = counter++;
            }

            //job->function(pool, running_thread, job->arg);
            gtpoolrr_job_run(pool, index, running_thread, job, 0);
            running_thread = NULL;
        }

        printf("Completed job %lu:%lu\n", index, (size_t) job->user_tag);
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
    destructor_arg->desired_state = desired_state;
    destructor_arg->current_state = current_state;
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
           (1 * vpool_advise(thread_count * jobs_per_thread, sizeof(struct gtpoolrr_job))) +
           (1 * vqueue_advisev(thread_count, jobs_per_thread, sizeof(Greent *))) +
           (thread_count * sizeof(struct gtpoolrr_worker_arg)) +
           (1 * aqueue_advisev(thread_count, sizeof(struct gtpoolrr_job*), jobs_per_thread)) + // submissions
           (1 * aqueue_advisev(thread_count, sizeof(struct gtpoolrr_job*), jobs_per_thread)) + // completions
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
        pool->jobs = ptr;
        ptr = pointer_literal_addition(ptr, vpool_advise(thread_count * jobs_per_thread, sizeof(struct gtpoolrr_job)));
        pool->ready_threads = ptr;
        ptr = pointer_literal_addition(ptr, vqueue_advise(thread_count * jobs_per_thread, sizeof(Greent*)));
        pool->worker_args = ptr;
        ptr = pointer_literal_addition(ptr, thread_count * sizeof(struct gtpoolrr_worker_arg));
        pool->job_submission_queues = ptr;
        ptr = pointer_literal_addition(ptr, 1 * aqueue_advisev(thread_count, sizeof(struct gtpoolrr_job*), jobs_per_thread));
        pool->job_completion_queues = ptr;
        ptr = pointer_literal_addition(ptr, 1 * aqueue_advisev(thread_count, sizeof(struct gtpoolrr_job*), jobs_per_thread));
        pool->desired_states = ptr;
        ptr = pointer_literal_addition(ptr, thread_count * sizeof(_Atomic enum gtpoolrr_thread_state));
        pool->current_states = ptr;
        ptr = pointer_literal_addition(ptr, thread_count * sizeof(_Atomic enum gtpoolrr_thread_state));
    }

    // Actually initialize
    {
        res = vpool_init(&(pool->jobs), pool->jobs, thread_count * jobs_per_thread, sizeof(struct gtpoolrr_job), VPOOL_KIND_STATIC);
        if(res != 0) {
            return res;
        }
        res = vqueue_initv(thread_count, &(pool->ready_threads), pool->ready_threads, sizeof(Greent*), jobs_per_thread);
        if(res != 0) {
            return res;
        }
        res = aqueue_initv(thread_count, &(pool->job_submission_queues), (void *) pool->job_submission_queues, sizeof(struct gtpoolrr_job*), jobs_per_thread);
        if(res != 0) {
            return res;
        }
        res = aqueue_initv(thread_count, &(pool->job_completion_queues), (void *) pool->job_completion_queues, sizeof(struct gtpoolrr_job*), jobs_per_thread);
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

int gtpoolrr_sbs_push(Gtpoolrr *pool, struct gtpoolrr_job *job) {
    return gtpoolrr_sbs_push_rr(pool, job);
}

int gtpoolrr_sbs_pushall( Gtpoolrr *pool, size_t *num_pushed, size_t num_to_push, struct gtpoolrr_job *jobs[]) {
    return gtpoolrr_sbs_pushall_rr(pool, num_pushed, num_to_push, jobs);
}

int gtpoolrr_sbs_push_rr(Gtpoolrr *pool, struct gtpoolrr_job *job) {
    int res;

    if(pool == NULL || job->function == NULL || job->arg == NULL) {
        return EINVAL;
    }

    for(size_t i = 0; i <= pool->threads_total; i++) {
        size_t index = (i + pool->rr_index) % pool->threads_total;

        res = gtpoolrr_sbs_push_direct(pool, index, job);
        if(res != 0) {
            goto gtpoolrr_sbs_push_rr_error;
        } else if(res == EXFULL) {
            // try again for next
            continue;
        }

        pool->rr_index += 1;
        return 0;
    }

    // tried to push to all threads and failed
    res = EBUSY;

gtpoolrr_sbs_push_rr_error:
    return res;
}

int gtpoolrr_sbs_pushall_rr(Gtpoolrr *pool, size_t *num_pushed, size_t num_to_push, struct gtpoolrr_job *jobs[]) {
    int res;
    if(pool == NULL || num_pushed == NULL || jobs == NULL) {
        return EINVAL;
    }

    *num_pushed = 0;
    for(size_t i = 0; i < num_to_push; i++){
        res = gtpoolrr_sbs_push_rr(pool, jobs[i]);
        if(res != 0){
            return res;
        }

        *num_pushed += 1;
    }

    return 0;
}

int gtpoolrr_sbs_push_direct(Gtpoolrr *pool, size_t thread_index, struct gtpoolrr_job *job) {
    if(pool == NULL || job->function == NULL || job->arg == NULL) {
        return EINVAL;
    }

    int res = aqueue_enqueue(&(pool->job_submission_queues[thread_index]), &job);
    if(res != 0) {
        return res;
    }

    return 0;
}

int gtpoolrr_sbs_pushall_direct(
    Gtpoolrr *pool, size_t thread_index, size_t *num_pushed, size_t num_to_push,
    struct gtpoolrr_job *jobs[]
) {
    int res;
    if(pool == NULL || num_pushed == NULL || jobs == NULL) {
        return EINVAL;
    }

    *num_pushed = 0;
    for(size_t i = 0; i < num_to_push; i++){
        res = gtpoolrr_sbs_push_direct(pool, thread_index, jobs[i]);
        if(res != 0){
            return res;
        }

        *num_pushed += 1;
    }

    return 0;
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

int gtpoolrr_sbs_get(
    Gtpoolrr *thread_pool, struct gtpoolrr_job *jobs_dest[],
    size_t *num_obtained, size_t num_requested
) {
    struct gtpoolrr_job *job;
    if(thread_pool == NULL || jobs_dest == NULL || num_obtained == NULL || num_requested == 0) {
        return EINVAL;
    }

    *num_obtained = 0;

    for(size_t i = 0; i < num_requested; i++) {
        job = vpool_alloc(thread_pool->jobs);
        memset(job, 0, sizeof(struct gtpoolrr_job));
        if(job == NULL){
            return ENODATA;
        }

        jobs_dest[i] = job;
        *num_obtained += 1;
    }

    return 0;
}

void gtpoolrr_cps_ack(Gtpoolrr *thread_pool, struct gtpoolrr_job *done_jobs[], size_t num_acknowledged) {
    if(thread_pool == NULL || done_jobs == NULL || num_acknowledged == 0) {
        return;
    }

    for(size_t i = 0; i < num_acknowledged; i++) {
        if(vpool_dealloc(thread_pool->jobs, done_jobs[i]) != 0){
            assert(false);
        }
    }

    return;
}

void gtpoolrr_sbs_set_tag(struct gtpoolrr_job *job, uint64_t user_tag) {
    if(job == NULL) {
        return;
    }

    job->user_tag = user_tag;
    return;
}

void gtpoolrr_sbs_set_function(
    struct gtpoolrr_job *job,
    void *((*function)(volatile Gtpoolrr*, volatile Greent*, volatile void*))
) {
    if(job == NULL) {
        return;
    }

    job->function = function;
    return;
}
void gtpoolrr_sbs_set_arg(struct gtpoolrr_job *job, void *arg) {
    if(job == NULL) {
        return;
    }

    job->arg = arg;
    return;
}

void gtpoolrr_sbs_set_expiration(struct gtpoolrr_job *job, uint64_t nanoseconds_from_now) {
    if(job == NULL) {
        return;
    }

    job->expiration = gt_monotonic_time_now() + nanoseconds_from_now;
    return;
}

void gtpoolrr_sbs_set_tags(struct gtpoolrr_job *jobs[], size_t num_jobs_to_set_for, uint64_t user_tag) {
    if(jobs == NULL || num_jobs_to_set_for == 0) {
        return;
    }

    for(size_t i = 0; i < num_jobs_to_set_for; i++) {
        jobs[i]->user_tag = user_tag;
    }

    return;
}

void gtpoolrr_sbs_set_functions(
    struct gtpoolrr_job *jobs[], size_t num_jobs_to_set_for,
    void *((*function)(volatile Gtpoolrr*, volatile Greent*, volatile void*))
) {
    if(jobs == NULL || num_jobs_to_set_for == 0) {
        return;
    }

    for(size_t i = 0; i < num_jobs_to_set_for; i++) {
        jobs[i]->function = function;
    }

    return;
}
void gtpoolrr_sbs_set_args(struct gtpoolrr_job *jobs[], size_t num_jobs_to_set_for, void *arg) {
    if(jobs == NULL || num_jobs_to_set_for == 0) {
        return;
    }

    for(size_t i = 0; i < num_jobs_to_set_for; i++) {
        jobs[i]->arg = arg;
    }

    return;
}
void gtpoolrr_sbs_set_expirations(
    struct gtpoolrr_job *jobs[], size_t num_jobs_to_set_for, uint64_t nanoseconds_from_now
) {
    if(jobs == NULL || num_jobs_to_set_for == 0) {
        return;
    }

    for(size_t i = 0; i < num_jobs_to_set_for; i++) {
        jobs[i]->expiration = gt_monotonic_time_now() + nanoseconds_from_now;
    }

    return;
}

int gtpoolrr_cps_pop(Gtpoolrr *pool, struct gtpoolrr_job **dest) {
    struct gtpoolrr_job *job;
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
            *dest = job;
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

int gtpoolrr_cps_popsome(Gtpoolrr *pool, struct gtpoolrr_job *dest[], size_t *num_popped, size_t at_most) {
    int res = 0;
    size_t index = 0;
    if(pool == NULL || dest == NULL || at_most == 0 || num_popped == NULL) {
        res = EINVAL;
        goto gtpoolrr_cps_popsome_end;
    }

    *num_popped = 0;
    while(index < at_most) {
        res = gtpoolrr_cps_pop(pool, &(dest[index]));

        if(res == 0) {
            index++;
        } else {
            goto gtpoolrr_cps_popsome_end;
        }
    }

gtpoolrr_cps_popsome_end:
    *num_popped = index;
    return res;
}

int gtpoolrr_cps_popall(Gtpoolrr *pool, struct gtpoolrr_job *dest[], size_t wait_for) {
    int res = 0;
    if(pool == NULL || dest == NULL || wait_for == 0) {
        res = EINVAL;
        goto gtpoolrr_cps_popall_end;
    }

    size_t i = 0;
    while(i < wait_for) {
        const int gtpoolrr_cps_pop_res = gtpoolrr_cps_pop(pool, &(dest[i]));

        if(gtpoolrr_cps_pop_res == 0) {
            // pass
            i++;
        } else if(gtpoolrr_cps_pop_res == EBUSY) {
            usleep(1000);
            continue;
        } else {
            res = gtpoolrr_cps_pop_res;
            goto gtpoolrr_cps_popall_end;
        }
    }

gtpoolrr_cps_popall_end:
    return res;
}

size_t gtpoolrr_sbs_queued(Gtpoolrr *pool) {
    size_t total = 0;

    if(pool == NULL) {
        return 0;
    }

    for(size_t i = 0; i < pool->threads_total; i++) {
        total += aqueue_len(&(pool->job_submission_queues[i]));
    }

    return total;
}

size_t gtpoolrr_sbs_empty(Gtpoolrr *pool) {
    size_t total = 0;

    if(pool == NULL) {
        return 0;
    }

    for(size_t i = 0; i < pool->threads_total; i++) {
        total += aqueue_cap(&(pool->job_submission_queues[i])) - aqueue_len(&(pool->job_submission_queues[i]));
    }

    return total;
}

size_t gtpoolrr_sbs_cap(Gtpoolrr *pool) {
    if(pool == NULL) {
        return 0;
    }

    return pool->threads_total * pool->jobs_per_thread;
}

size_t gtpoolrr_cps_queued(Gtpoolrr *pool) {
    size_t total = 0;

    if(pool == NULL) {
        return 0;
    }

    for(size_t i = 0; i < pool->threads_total; i++) {
        total += aqueue_len(&(pool->job_completion_queues[i]));
    }

    return total;
}

size_t gtpoolrr_cps_empty(Gtpoolrr *pool) {
    size_t total = 0;

    if(pool == NULL) {
        return 0;
    }

    for(size_t i = 0; i < pool->threads_total; i++) {
        total += aqueue_cap(&(pool->job_completion_queues[i])) - aqueue_len(&(pool->job_completion_queues[i]));
    }

    return total;
}

size_t gtpoolrr_cps_cap(Gtpoolrr *pool) {
    if(pool == NULL) {
        return 0;
    }

    return pool->threads_total * pool->jobs_per_thread;
}
