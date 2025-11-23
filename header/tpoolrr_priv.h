#include <inttype.h>

// Represents a single job that should be performed by one of the threads
struct tpoolrr_job {
    void (*function)(void*);
    void *arg;
    uint64_t expiration;
}

// The type used as an argument to consumer_function
struct consumer_arg {
    Aqueue *job_queue;
    sem_t *semaphore;
}

// Monotonic time in milliseconds
uint64_t monotonic_time_now();

// Returns job construct assembled from arguments
struct tpoolrr_job_construct(void (*function)(void*), void *arg, size_t expiration);

// Function spawned on new thread when initalizing/creating the thread pool
void *consumer_loop_function(void *void_arg);
