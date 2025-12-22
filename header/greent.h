#pragma once

#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <liburing.h>

#define GREENT_DO_NOP (0)
#define GREENT_DO_IOURING_READ (1)
#define GREENT_DO_IOURING_WRITE (2)
#define GREENT_DO_IOURING_OPEN (3)
#define GREENT_DO_IOURING_CLOSE (4)
#define GREENT_DO_IOURING_TIMEOUT (64)
#define GREENT_DO_IOURING_READT (65)
#define GREENT_DO_IOURING_WRITET (66)
#define GREENT_DO_IOURING_OPENT (67)
#define GREENT_DO_IOURING_CLOSET (68)

// struct representing a yet to start asynchronous operation (like sleeping, reading, or opening file)
typedef struct greent_do_submission {
    // the thread that asked to make an async submission
    uint64_t user_data;

    // Operation and args to use with asynchronous function
    uint64_t do_this;
    uint64_t arg1;
    uint64_t arg2;
    uint64_t arg3;
    uint64_t arg4;
    uint64_t arg5;
    uint64_t arg6;
    uint64_t arg7;
    uint64_t arg8;

    union {
        // some submission functions require pointers rather than literals
        mode_t mode;
        struct __kernel_timespec ts;
    };
} Greent_do_submission;

// Struct representing a completed asynchronous operation (like sleeping, reading, or opening file)
// Similar API to io_uring_completion struct
typedef struct greent_do_completion {
    // unique 64-bit integer provided on submission.
    uint64_t user_data;

    // return code from trying to perform asynchronous operation.
    uint64_t res;

    // various flags set associated with this.
    uint64_t flags;
} Greent_do_completion;

// Parent to all green threads for a given software thread
typedef struct vert {
    // r12 to r15 registers
    size_t r12;
    size_t r13;
    size_t r14;
    size_t r15;

    // other caller saved registers
    size_t rbx;
    void *rsp;
    void *rbp;
    void *rip;

    // where the first invocation of prepare_root is meant to return to
    void *first_called_from;
} Vert;

typedef struct green {
    // r12 to r15 registers
    size_t r12;
    size_t r13;
    size_t r14;
    size_t r15;

    // caller-saved register
    size_t rbx;

    // stack pointer register
    void *rsp;

    // base pointer register
    void *rbp;
 
    // instruction pointer register
    void *rip;

    // stack_frames
    char stack_frames[1024];

    // number of bytes composing all stack frames between greent_spawn to yield call
    size_t top_of_frame;

    // the parent Vert that manages this green thread
    Vert *parent;

    // The flags used to store information related to the request
    // LSB_0 | LSB_1 | LSB_2 | ... | LSB_63
    uint64_t flags;

    // Unique thread id
    uint64_t unique_id;

    // Submission struct
    Greent_do_submission submission;

    // Completion struct
    Greent_do_completion completion;
} Greent;

// Standard API
size_t greent_advise();
size_t greent_advisev(size_t num_threads);
// Both init and initv expect memory to be aligned to a multiple of 8
int greent_init(Greent *green_thread, Vert *parent, uint64_t unique_id);
int greent_initv(size_t num_threads, Greent **dest, void *memory, Vert *parent, uint64_t unique_id);

// Check whether this green thread has already been initialized
bool greent_initalized(Greent *green_thread);

// Tag pointer when attaching to io_uring submission
int greent_pack(
    __u64 *tagged_ptr,
    Greent *green_thread, bool with_timeout, bool something, bool something_else
);
// Convert tagged pointer to normal pointer and determine whether status bits are set
int greent_unpack(
    __u64 tagged_ptr,
    Greent **green_thread, bool *with_timeout, bool *something, bool *something_else
);

// Give cpu time to other green threads while asynchronous operations are performed
uint64_t greent_do_nop(volatile Greent *green_thread);
// Mirrors Linux read system call
uint64_t greent_do_read(
    volatile Greent *green_thread,
    volatile int fd, volatile void *buf, volatile unsigned nbyes, volatile uint64_t offset
);
// Mirrors Linux write system call
uint64_t greent_do_write(
    volatile Greent *green_thread,
    volatile int fd, volatile void *buf, volatile unsigned nbyes, volatile uint64_t offset
);
// Mirrors Linux open system call
uint64_t greent_do_open(
    volatile Greent *green_thread,
    volatile char *path, volatile int flags, volatile mode_t mode
);
// Mirrors Linux close system call
uint64_t greent_do_close(volatile Greent *green_thread, volatile int fd);

// Mirrors Linux read system call with added timeout
uint64_t greent_do_readt(
    volatile Greent *green_thread,
    volatile int fd, volatile void *buf, volatile unsigned nbyes, volatile uint64_t offset,
    __kernel_time64_t tv_sec, long long tv_nsec
);
// Mirrors Linux write system call with added timeout
uint64_t greent_do_writet(
    volatile Greent *green_thread,
    volatile int fd, volatile void *buf, volatile unsigned nbyes, volatile uint64_t offset,
    __kernel_time64_t tv_sec, long long tv_nsec
);

// Used internally for working with green thread asynchronous operations
void greent_do_submit(Greent *green_thread, struct io_uring *ring, size_t *num_submissions);

// Internal
// Needs to be assembly in order to set registers
extern __attribute__((__naked__, noinline))
Greent *greent_root(Vert *root);
// Needs to be assembly in order to set registers
extern __attribute__((__naked__, noinline))
uint64_t greent_yield(volatile Greent *buf);
// Needs to be assembly in order to set registers
extern __attribute__((__naked__, indirect_return, noinline))
_Noreturn void greent_resume(volatile Greent *buf, volatile uint64_t retval);
