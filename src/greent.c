#include "greent.h"

#include <string.h>
#include <setjmp.h>
#include <stdio.h>
#include <assert.h>
#include <stdnoreturn.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <liburing.h>

size_t greent_advise() {
    return 1 * (sizeof(Greent));
}

size_t greent_advisev(size_t num_threads) {
    return num_threads * (sizeof(Greent));
}

int greent_init(Greent *green_thread, Vert *parent, uint64_t unique_id) {
    if(green_thread == NULL || parent == NULL) {
        return EINVAL;
    }

    if(memset(green_thread, 0, greent_advise()) != 0) {
        return ENOTRECOVERABLE;
    }
    green_thread->parent = parent;
    green_thread->unique_id = unique_id;

    return 0;
}

int greent_initv(size_t num_threads, Greent **dest, void *memory, Vert *parent, uint64_t unique_id) {
    Greent *green_threads;
    if(num_threads == 0 || dest == NULL || memory == NULL || parent == NULL) {
        return EINVAL;
    }

    green_threads = memory;
    if(memset(green_threads, 0, greent_advisev(num_threads)) != 0) {
        return ENOTRECOVERABLE;
    }
    for(size_t i = 0; i < num_threads; i++) {
        green_threads[i].parent = parent;
        green_threads[i].unique_id = unique_id;
    }

    *dest = green_threads;
    return 0;
}

bool greent_initalized(Greent *green_thread) {
    return (green_thread != NULL) && (green_thread->parent != NULL);
}

int greent_pack(
    __u64 *tagged_ptr,
    Greent *green_thread, bool with_timeout, bool something, bool something_else
) {
    __u64 green_thread_u64 = 0;
    __u64 with_timeout_u64 = 0;
    __u64 something_u64 = 0;
    __u64 something_else_u64 = 0;
    if(tagged_ptr == NULL || green_thread == NULL){
        return EINVAL;
    }

    green_thread_u64 = (__u64) green_thread;
    assert((green_thread_u64 % 8) == 0); // make sure green_thread is aligned
    if(with_timeout) { with_timeout_u64 = 1 << 2; }
    if(something) { something_u64 = 1 << 1; }
    if(something_else) { something_else_u64 = 1 << 0; }
    *tagged_ptr = green_thread_u64 | with_timeout_u64 | something_u64 | something_else_u64;

    return 0;
}

int greent_unpack(
    __u64 tagged_ptr,
    Greent **green_thread, bool *with_timeout, bool *something, bool *something_else
) {
    if(tagged_ptr == 0) {
        return EINVAL; // tagged_ptr is a NULL pointer
    }

    const __u64 with_timeout_mask = 1 << 2;
    const __u64 something_mask = 1 << 1;
    const __u64 something_else_mask = 1 << 0;
    const __u64 green_thread_mask = (
        ((__u64) 0xffffffffffffffffllu) ^ (with_timeout_mask | something_mask | something_else_mask)
    );

    *green_thread = (Greent *) (tagged_ptr & green_thread_mask);
    *with_timeout = tagged_ptr & with_timeout_mask;
    *something = tagged_ptr & something_mask;
    *something_else = tagged_ptr & something_else_mask;

    return 0;
}

uint64_t greent_do_nop(volatile Greent *green_thread) {
    if(green_thread == NULL) {
        return EINVAL;
    }

    green_thread->submission = (Greent_do_submission) {
        0
    };
    green_thread->submission.user_data = (__u64) green_thread;
    green_thread->submission.do_this = GREENT_DO_NOP;

    return greent_yield(green_thread);
}

uint64_t greent_do_read(
    volatile Greent *green_thread,
    volatile int fd, volatile void *buf, volatile unsigned nbytes, volatile uint64_t offset
) {
    if(green_thread == NULL) {
        return EINVAL;
    }

    green_thread->submission = (Greent_do_submission) {
        0
    };
    green_thread->submission.user_data = (__u64) green_thread;
    green_thread->submission.do_this = GREENT_DO_IOURING_READ;
    green_thread->submission.arg1 = (uint64_t) fd;
    green_thread->submission.arg2 = (uint64_t) buf;
    green_thread->submission.arg3 = (uint64_t) nbytes;
    green_thread->submission.arg4 = (uint64_t) offset;

    return greent_yield(green_thread);
}

uint64_t greent_do_write(
    volatile Greent *green_thread,
    volatile int fd, volatile void *buf, volatile unsigned nbytes, volatile uint64_t offset
) {
    if(green_thread == NULL) {
        return EINVAL;
    }

    green_thread->submission = (Greent_do_submission) {
        0
    };
    green_thread->submission.user_data = (__u64) green_thread;
    green_thread->submission.do_this = GREENT_DO_IOURING_WRITE;
    green_thread->submission.arg1 = (uint64_t) fd;
    green_thread->submission.arg2 = (uint64_t) buf;
    green_thread->submission.arg3 = (uint64_t) nbytes;
    green_thread->submission.arg4 = (uint64_t) offset;

    return greent_yield(green_thread);
}

uint64_t greent_do_open(
    volatile Greent *green_thread,
    volatile char *path, volatile int flags, volatile mode_t mode
) {
    if(green_thread == NULL) {
        return EINVAL;
    }

    green_thread->submission = (Greent_do_submission) {
        0
    };

    green_thread->submission.mode = mode;

    green_thread->submission.user_data = (__u64) green_thread;
    green_thread->submission.do_this = GREENT_DO_IOURING_OPEN;
    green_thread->submission.arg1 = (uint64_t) path;
    green_thread->submission.arg2 = (uint64_t) flags;
    green_thread->submission.arg3 = (uint64_t) &(green_thread->submission.mode);

    return greent_yield(green_thread);
}

uint64_t greent_do_close(volatile Greent *green_thread, volatile int fd) {
    if(green_thread == NULL) {
        return EINVAL;
    }

    green_thread->submission = (Greent_do_submission) {
        0
    };
    green_thread->submission.user_data = (__u64) green_thread;
    green_thread->submission.do_this = GREENT_DO_IOURING_CLOSE;
    green_thread->submission.arg1 = (uint64_t) fd;

    return greent_yield(green_thread);
}

uint64_t greent_do_readt(
    volatile Greent *green_thread,
    volatile int fd, volatile void *buf, volatile unsigned nbytes, volatile uint64_t offset,
    __kernel_time64_t tv_sec, long long tv_nsec
) {
    if(green_thread == NULL) {
        return EINVAL;
    }

    green_thread->submission = (Greent_do_submission) {
        0
    };

    green_thread->submission.ts = (struct __kernel_timespec) { 0 };
    green_thread->submission.ts.tv_sec = tv_sec;
    green_thread->submission.ts.tv_nsec = tv_nsec;

    green_thread->submission.user_data = (__u64) green_thread;
    green_thread->submission.do_this = GREENT_DO_IOURING_READT;
    green_thread->submission.arg1 = (uint64_t) fd;
    green_thread->submission.arg2 = (uint64_t) buf;
    green_thread->submission.arg3 = (uint64_t) nbytes;
    green_thread->submission.arg4 = (uint64_t) offset;

    return greent_yield(green_thread);
}

uint64_t greent_do_writet(
    volatile Greent *green_thread,
    volatile int fd, volatile void *buf, volatile unsigned nbytes, volatile uint64_t offset,
    __kernel_time64_t tv_sec, long long tv_nsec
) {
    if(green_thread == NULL) {
        return EINVAL;
    }

    green_thread->submission = (Greent_do_submission) {
        0
    };

    green_thread->submission.ts = (struct __kernel_timespec) { 0 };
    green_thread->submission.ts.tv_sec = tv_sec;
    green_thread->submission.ts.tv_nsec = tv_nsec;

    green_thread->submission.user_data = (__u64) green_thread;
    green_thread->submission.do_this = GREENT_DO_IOURING_WRITET;
    green_thread->submission.arg1 = (uint64_t) fd;
    green_thread->submission.arg2 = (uint64_t) buf;
    green_thread->submission.arg3 = (uint64_t) nbytes;
    green_thread->submission.arg4 = (uint64_t) offset;

    return greent_yield(green_thread);
}

void greent_do_submit(Greent *green_thread, struct io_uring *ring, size_t *num_submissions) {
    int res;
    struct io_uring_sqe *sqe;
    if(green_thread == NULL || ring == NULL || num_submissions == NULL) {
        return;
    }

    // make sure to zero this out
    *num_submissions = 0;

    // name what is used to contruct request
    int fd;
    void *buf;
    unsigned num_bytes;
    __u64 offset_into;
    char *filename;
    int flags;
    mode_t mode;

    switch(green_thread->submission.do_this) {
    case GREENT_DO_NOP:
        break;
    case GREENT_DO_IOURING_READ:
        sqe = io_uring_get_sqe(ring);
        res = greent_pack(&(sqe->user_data), green_thread, false, false, false);
        assert(res == 0);
        sqe->flags = 0;

        fd = (int) green_thread->submission.arg1;
        buf = (void *) green_thread->submission.arg2;
        num_bytes = (unsigned) green_thread->submission.arg3;
        offset_into = (__u64) green_thread->submission.arg4;

        io_uring_prep_read(sqe, fd, buf, num_bytes, offset_into);
        io_uring_submit(ring);
        *num_submissions += 1;
        break;
    case GREENT_DO_IOURING_READT:
        // read request itself
        // need to make sure sqe->flags is bitwise ORed with IOSQE_IO_LINK for timeout
        {
            sqe = io_uring_get_sqe(ring);
            res = greent_pack(&(sqe->user_data), green_thread, false, false, false);
            assert(res == 0);
            sqe->flags = 0 | IOSQE_IO_LINK;

            fd = (int) green_thread->submission.arg1;
            buf = (void *) green_thread->submission.arg2;
            num_bytes = (unsigned) green_thread->submission.arg3;
            offset_into = (__u64) green_thread->submission.arg4;

            io_uring_prep_read(sqe, fd, buf, num_bytes, offset_into);
            *num_submissions += 1;
        }

        // timeout is created here
        // the cqe associated with it will be ignored but the timeout is able to cancel the first read request
        {
            sqe = io_uring_get_sqe(ring);
            res = greent_pack(&(sqe->user_data), green_thread, true, false, false);
            assert(res == 0);
            sqe->flags = 0;
            io_uring_prep_link_timeout(sqe, &(green_thread->submission.ts), 0);
            *num_submissions += 1;
        }

        io_uring_submit(ring);
        break;
    case GREENT_DO_IOURING_WRITE:
        sqe = io_uring_get_sqe(ring);
        res = greent_pack(&(sqe->user_data), green_thread, false, false, false);
        assert(res == 0);
        sqe->flags = 0;

        fd = (int) green_thread->submission.arg1;
        buf = (void *) green_thread->submission.arg2;
        num_bytes = (unsigned) green_thread->submission.arg3;
        offset_into = (__u64) green_thread->submission.arg4;

        io_uring_prep_write(sqe, fd, buf, num_bytes, offset_into);
        *num_submissions += 1;
        io_uring_submit(ring);
        break;
    case GREENT_DO_IOURING_WRITET:
        // write request itself
        // need to make sure sqe->flags is bitwise ORed with IOSQE_IO_LINK for timeout
        {
            sqe = io_uring_get_sqe(ring);
            res = greent_pack(&(sqe->user_data), green_thread, false, false, false);
            assert(res == 0);
            sqe->flags = 0 | IOSQE_IO_LINK;

            fd = (int) green_thread->submission.arg1;
            buf = (void *) green_thread->submission.arg2;
            num_bytes = (unsigned) green_thread->submission.arg3;
            offset_into = (__u64) green_thread->submission.arg4;

            io_uring_prep_write(sqe, fd, buf, num_bytes, offset_into);
            *num_submissions += 1;
        }

        // timeout is created here
        // the cqe associated with it will be ignored but the timeout is able to cancel the first read request
        {
            sqe = io_uring_get_sqe(ring);
            res = greent_pack(&(sqe->user_data), green_thread, true, false, false);
            assert(res == 0);
            sqe->flags = 0;
            io_uring_prep_link_timeout(sqe, &(green_thread->submission.ts), 0);
            *num_submissions += 1;
        }

        io_uring_submit(ring);
        break;
    case GREENT_DO_IOURING_OPEN:
        sqe = io_uring_get_sqe(ring);
        res = greent_pack(&(sqe->user_data), green_thread, false, false, false);
        assert(res == 0);
        sqe->flags = 0;

        filename = (char *) green_thread->submission.arg1;
        flags = (int) green_thread->submission.arg2;
        mode = *((mode_t *) green_thread->submission.arg3);

        io_uring_prep_open(sqe, filename, flags, mode);
        io_uring_submit(ring);
        *num_submissions += 1;
        break;
    case GREENT_DO_IOURING_CLOSE:
        sqe = io_uring_get_sqe(ring);
        res = greent_pack(&(sqe->user_data), green_thread, false, false, false);
        assert(res == 0);
        sqe->flags = 0;

        fd = (int) green_thread->submission.arg1;

        io_uring_prep_close(sqe, fd);
        io_uring_submit(ring);
        *num_submissions += 1;
        break;
    default:
        assert(false);
        break;
    }

    return;
}
