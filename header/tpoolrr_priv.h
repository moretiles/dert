// Represents a single job that should be performed by one of the threads
struct tpoolrr_job {
    void *((*function)(void*));
    void *arg;
    uint64_t expiration;
};

// The type used as an argument to tpoolrr_worker
struct tpoolrr_worker_arg {
    Tpoolrr *pool;
    size_t index;
};

// Monotonic time in milliseconds
uint64_t monotonic_time_now();

// Returns job construct assembled from arguments
struct tpoolrr_job tpoolrr_job_construct(void *((*start_routine) (void*)), void *arg, size_t expiration);

struct tpoolrr_worker_arg tpoolrr_worker_arg_construct(Tpoolrr *pool, size_t index);

// Function spawned on new thread when initalizing/creating the thread pool
void *tpoolrr_worker(void *void_arg);

int _tpoolrr_jobs_add(Tpoolrr *pool, struct tpoolrr_job job);
