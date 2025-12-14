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

#define GREENT_COUNT (3)

Vert vert = { 0 };
Greent *waiting_green_threads;
Greent *ready_green_threads;
Greent *running_green_thread;

Greent_do_submission *submission_queue;
Greent_do_completion *completion_queue;

uint64_t greent_do_nop(volatile Greent *green_thread, volatile uint64_t user_data) {
    if(green_thread == NULL) {
        return 1;
    }

    green_thread->submission = (Greent_do_submission) {
        0
    };
    green_thread->submission.user_data = user_data;
    green_thread->submission.do_this = GREENT_DO_NOP;

    return greent_yield(green_thread);
}

uint64_t greent_do_read(volatile Greent *green_thread, volatile uint64_t user_data, volatile int fd, volatile void *buf, volatile unsigned nbytes, volatile uint64_t offset) {
    if(green_thread == NULL) {
        return 1;
    }

    green_thread->submission = (Greent_do_submission) {
        0
    };
    green_thread->submission.user_data = user_data;
    green_thread->submission.do_this = GREENT_DO_IOURING_READ;
    green_thread->submission.arg1 = (uint64_t) fd;
    green_thread->submission.arg2 = (uint64_t) buf;
    green_thread->submission.arg3 = (uint64_t) nbytes;
    green_thread->submission.arg4 = (uint64_t) offset;

    return greent_yield(green_thread);
}

uint64_t greent_do_write(volatile Greent *green_thread, volatile uint64_t user_data, volatile int fd, volatile void *buf, volatile unsigned nbytes, volatile uint64_t offset) {
    if(green_thread == NULL) {
        return 1;
    }

    green_thread->submission = (Greent_do_submission) {
        0
    };
    green_thread->submission.user_data = user_data;
    green_thread->submission.do_this = GREENT_DO_IOURING_WRITE;
    green_thread->submission.arg1 = (uint64_t) fd;
    green_thread->submission.arg2 = (uint64_t) buf;
    green_thread->submission.arg3 = (uint64_t) nbytes;
    green_thread->submission.arg4 = (uint64_t) offset;

    return greent_yield(green_thread);
}

uint64_t greent_do_open(volatile Greent *green_thread, volatile uint64_t user_data, volatile char *path, volatile int flags, volatile mode_t *mode_ptr) {
    if(green_thread == NULL || mode_ptr == NULL) {
        return 1;
    }

    green_thread->submission = (Greent_do_submission) {
        0
    };
    green_thread->submission.user_data = user_data;
    green_thread->submission.do_this = GREENT_DO_IOURING_OPEN;
    green_thread->submission.arg1 = (uint64_t) path;
    green_thread->submission.arg2 = (uint64_t) flags;
    green_thread->submission.arg3 = (uint64_t) mode_ptr;

    return greent_yield(green_thread);
}

uint64_t greent_do_close(volatile Greent *green_thread, volatile uint64_t user_data, volatile int fd) {
    if(green_thread == NULL) {
        return 1;
    }

    green_thread->submission = (Greent_do_submission) {
        0
    };
    green_thread->submission.user_data = user_data;
    green_thread->submission.do_this = GREENT_DO_IOURING_CLOSE;
    green_thread->submission.arg1 = (uint64_t) fd;

    return greent_yield(green_thread);
}

void greent_do_submit(volatile Greent *green_thread, struct io_uring *ring) {
    struct io_uring_sqe *sqe;
    if(green_thread == NULL || ring == NULL) {
        return;
    }

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
        sqe->user_data = green_thread->submission.user_data;

        fd = (int) green_thread->submission.arg1;
        buf = (void *) green_thread->submission.arg2;
        num_bytes = (unsigned) green_thread->submission.arg3;
        offset_into = (__u64) green_thread->submission.arg4;

        io_uring_prep_read(sqe, fd, buf, num_bytes, offset_into);
        io_uring_submit(ring);
        break;
    case GREENT_DO_IOURING_WRITE:
        sqe = io_uring_get_sqe(ring);
        sqe->user_data = green_thread->submission.user_data;

        fd = (int) green_thread->submission.arg1;
        buf = (void *) green_thread->submission.arg2;
        num_bytes = (unsigned) green_thread->submission.arg3;
        offset_into = (__u64) green_thread->submission.arg4;

        io_uring_prep_write(sqe, fd, buf, num_bytes, offset_into);
        io_uring_submit(ring);
        break;
    case GREENT_DO_IOURING_OPEN:
        sqe = io_uring_get_sqe(ring);
        sqe->user_data = green_thread->submission.user_data;

        filename = (char *) green_thread->submission.arg1;
        flags = (int) green_thread->submission.arg2;
        mode = *((mode_t *) green_thread->submission.arg3);

        io_uring_prep_open(sqe, filename, flags, mode);
        io_uring_submit(ring);
        break;
    case GREENT_DO_IOURING_CLOSE:
        sqe = io_uring_get_sqe(ring);
        sqe->user_data = green_thread->submission.user_data;

        fd = (int) green_thread->submission.arg1;

        io_uring_prep_close(sqe, fd);
        io_uring_submit(ring);
        break;
    default:
        assert(false);
        break;
    }

    return;
}

/*
void print_and_yield(volatile Greent *green_thread, volatile int n) {
    if(green_thread == NULL) {
        return;
    }

    printf("function called: %d\n", n);
    uint64_t ret = greent_do_nop(green_thread, n);
    printf("yield returned: %d\n", (int) ret);
}

uint8_t fib_with_yield(volatile Greent *green_thread, volatile uint8_t n) {
    uint8_t r1;
    uint8_t r2;

    if(n <= 1) {
        greent_do_nop(green_thread, n);
        return n;
    }

    // both work

    r1 = fib_with_yield(green_thread, n - 1);
    r2 = fib_with_yield(green_thread, n - 2);
    return r1 + r2;

    //return fib_with_yield(green_thread, n - 1) + fib_with_yield(green_thread, n - 2);
}

void cat_and_yield(volatile Greent *green_thread, volatile char *buf, volatile char *filename) {
    int fd;
    if(green_thread == NULL || filename == NULL) {
        goto cat_and_yield_end;
    }

    //fd = open((char *) filename, O_RDONLY);
    mode_t mode = 0600;
    greent_do_open(green_thread, green_thread->unique_id, filename, O_RDONLY, &mode);
    fd = green_thread->completion.res;
    if(fd <= 0) {
        printf("Error opening: %s\n", filename);
        goto cat_and_yield_end;
    }

    //int read_res = read(fd, buf, 1024 - 1);
    //while(read_res > 0){
        //read_res = read(fd, buf, 1024 - 1);
    //}

//    if(read_res == 0){
//        // everything has been read
//    } else {
//        perror("Got error when using read:");
//        errno = 0;
//        goto cat_and_yield_end;
//    }

    //greent_yield_nop(green_thread, green_thread->unique_id);
    greent_do_read(green_thread, green_thread->unique_id, fd, buf, 64 - 1, 0);
    const int read_res = green_thread->completion.res;
    if(read_res < 0) {
        errno = -read_res;
        printf("Error reading %s:", filename);
        perror("");
    }
    buf[green_thread->completion.res] = 0;

    printf("%s: %s", filename, buf);

cat_and_yield_end:
    if(fd > 0) {
        greent_do_close(green_thread, green_thread->unique_id, fd);
        if(green_thread->completion.res != 0) {
            // should never happen
            assert(false);
        }
    }
    return;
}

void write_and_yield(volatile Greent *green_thread, volatile char *buf, volatile char *filename) {
    int fd;
    if(green_thread == NULL || filename == NULL) {
        goto write_and_yield_end;
    }

    //fd = open((char *) filename, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    mode_t mode = 0600;
    greent_do_open(green_thread, green_thread->unique_id, filename, O_WRONLY | O_CREAT | O_TRUNC, &mode);
    fd = green_thread->completion.res;
    if(fd <= 0) {
        printf("Error opening: %s\n", filename);
        goto write_and_yield_end;
    }

//    int read_res = read(fd, buf, 1024 - 1);
//    while(read_res > 0){
//        read_res = read(fd, buf, 1024 - 1);
//    }
//
//    if(read_res == 0){
//        // everything has been read
//    } else {
//        perror("Got error when using read:");
//        errno = 0;
//        goto write_and_yield_end;
//    }

    //greent_yield_nop(green_thread, green_thread->unique_id);
    greent_do_write(green_thread, green_thread->unique_id, fd, buf, strnlen((char *) buf, 64), 0);
    const int write_res = green_thread->completion.res;
    if(write_res < 0) {
        errno = -write_res;
        printf("Error writing %s:", filename);
        perror("");
    }

    printf("Wrote %i bytes to: %s\n", write_res, filename);

write_and_yield_end:
    if(fd > 0) {
        greent_do_close(green_thread, green_thread->unique_id, fd);
        if(green_thread->completion.res != 0) {
            // should never happen
            assert(false);
        }
    }
    return;
}

void test_cat_and_yield(void) {
    volatile Greent greens[GREENT_COUNT];
    char *filenames[GREENT_COUNT] = { "./test/a", "./test/b", "./test/c" };
    volatile Greent *ret;
    volatile int n = 0;
    volatile int i = 0;
    struct io_uring ring;
    struct io_uring_cqe *cqe;
    char bufs[3][64];

    const int io_uring_queue_init_res = io_uring_queue_init(GREENT_COUNT, &ring, 0);
    if(io_uring_queue_init_res < 0) {
        return;
    }

    memset((void *) greens, 0, GREENT_COUNT * sizeof(Greent));
    memset(&vert, 0, sizeof(Vert));
    ret = greent_root(&vert);
    if(ret != NULL) {
        //greent_resume(ret, 444);
        greent_do_submit(ret, &ring);
        n++;
    }

    while (n < GREENT_COUNT) {
        greens[n].parent = &vert;
        greens[n].unique_id = n;
        cat_and_yield(&(greens[n]), bufs[n], filenames[n]);
        n++;
    }

    while (i++ < 3 * GREENT_COUNT) {
        //printf("Hello from the main function in C!\n");
        assert(io_uring_wait_cqe(&ring, &cqe) == 0);
        greens[cqe->user_data].completion.user_data = cqe->user_data;
        greens[cqe->user_data].completion.res = cqe->res;
        greens[cqe->user_data].completion.flags = cqe->flags;
        io_uring_cq_advance(&ring, 1);
        greent_resume(&(greens[cqe->user_data]), 0);
    }

    return;
}

void test_write_and_yield(void) {
    volatile Greent greens[GREENT_COUNT];
    char *filenames[GREENT_COUNT] = { "./test/d", "./test/e", "./test/f" };
    volatile Greent *ret;
    volatile int n = 0;
    volatile int i = 0;
    struct io_uring ring;
    struct io_uring_cqe *cqe;
    char bufs[3][64];
    strcpy(bufs[0], "This will be written to D!\n");
    strcpy(bufs[1], "This will be written to E!\n");
    strcpy(bufs[2], "This will be written to F!\n");

    const int io_uring_queue_init_res = io_uring_queue_init(GREENT_COUNT, &ring, 0);
    if(io_uring_queue_init_res < 0) {
        return;
    }

    memset((void *) greens, 0, GREENT_COUNT * sizeof(Greent));
    memset(&vert, 0, sizeof(Vert));
    ret = greent_root(&vert);
    if(ret != NULL) {
        //greent_resume(ret, 444);
        greent_do_submit(ret, &ring);
        n++;
    }

    while (n < GREENT_COUNT) {
        greens[n].parent = &vert;
        greens[n].unique_id = n;
        write_and_yield(&(greens[n]), bufs[n], filenames[n]);
        n++;
    }

    while (i++ < 3 * GREENT_COUNT) {
        //printf("Hello from the main function in C!\n");
        assert(io_uring_wait_cqe(&ring, &cqe) == 0);
        greens[cqe->user_data].completion.user_data = cqe->user_data;
        greens[cqe->user_data].completion.res = cqe->res;
        greens[cqe->user_data].completion.flags = cqe->flags;
        io_uring_cq_advance(&ring, 1);
        greent_resume(&(greens[cqe->user_data]), 0);
    }

    return;
}


void test_print_and_yield(void) {
    volatile Greent greens[5];
    volatile Greent *ret;
    volatile int n = 0;
    volatile int i = 0;

    memset((void *) greens, 0, 5 * sizeof(Greent));
    memset(&vert, 0, sizeof(Vert));
    ret = greent_root(&vert);
    if(ret != NULL) {
        //greent_resume(ret, 444);
        n++;
    }

    while (n < 5) {
        greens[n] = (Greent) {
            .parent = &vert
        };
        print_and_yield(&(greens[n]), 111 * (n + 1));
    }

    while (i < 3 * 5) {
        //printf("Hello from the main function in C!\n");
        i++;
        greent_resume(&(greens[(i - 1) % 5]), i);
    }

    return;
}

void test_main(void) {
    volatile Greent greens[GREENT_COUNT];
    char *filenames[GREENT_COUNT] = { "./test/a", "./test/b", "./test/c" };
    volatile Greent *ret;
    volatile int n = 0;
    volatile int i = 0;
    struct io_uring ring;
    struct io_uring_cqe *cqe;
    char bufs[3][64];

    const int io_uring_queue_init_res = io_uring_queue_init(GREENT_COUNT, &ring, 0);
    if(io_uring_queue_init_res < 0) {
        return;
    }

    memset((void *) greens, 0, GREENT_COUNT * sizeof(Greent));
    memset(&vert, 0, sizeof(Vert));
    ret = greent_root(&vert);
    if(ret != NULL) {
        greent_yield_submit(ret, &ring);
    }

    while(true){
        unsigned i = 0;
        io_uring_for_each_cqe(ring, head, cqe) {
            Greent *ready = move_from_waiting_to_ready(waiting);
            greens[cqe->user_data].completion.user_data = cqe->user_data;
            greens[cqe->user_data].completion.res = cqe->res;
            greens[cqe->user_data].completion.flags = cqe->flags;
            assert(ready != NULL);
            i++;
        }
        io_uring_cq_advance(ring, i);

        Greent *ready = pop_from_ready();
        if(not_started_yet(ready)) {
            greens[n].parent = &vert;
            greens[n].unique_id = n++;
            ready->function(ready, ready->arg);
        } else {
            greent_resume(ready, 0);
        }

        usleep(1000);
    }

    return;
}

int main(void) {
    test_print_and_yield();
    test_cat_and_yield();
    test_write_and_yield();
}

//int main(void) {
//    Greent yielded;
//
//    yielded = greent_root(&vert);
//    if(yielded == NULL){
//        // jumppoint itself was prepared
//    } else if(yielded->marker == 0) {
//        ready_jobs.enqueue(yielded)
//    } else {
//        submissions.enqueue(yielded);
//        waiting_jobs.enqueue(yielded);
//    }
//
//    while(true) {
//        while !(completions.is_empty()) {
//            completion = completions.dequeue()
//            move_thread_from_waiting_to_ready(completion)
//        }
//
//        running_job = ready_jobs.dequeue(yielded)
//        if(running_job.started()){
//            greent_resume(running_job)
//        } else {
//            running_job.execute()
//        }
//
//        // job finished rather than yielding
//        complete_job(running_job);
//    }
//}
*/
