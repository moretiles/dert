#include <gtpoolrr.h>

namespace {
struct gtpoolrr_test_arg1 {
    int n;
};

// Does simple print, yields to parent to do nop, does another print
void *gtpoolrr_test1(Gtpoolrr *volatile pool, Greent *volatile green_thread, void *volatile varg) {
    if(pool == NULL || green_thread == NULL || varg == NULL) {
        return NULL;
    }

    struct gtpoolrr_test_arg1 *volatile arg = (struct gtpoolrr_test_arg1*) varg;

    MT_LOG("function called: %d", arg->n);
    uint64_t ret = greent_do_nop(green_thread);
    MT_LOG("yield returned: %d", (int) ret);

    return NULL;
}

struct gtpoolrr_test_arg2 {
    char *buf;
    const char *filename;
};

// Cats out the first 63 bytes of a file
void *gtpoolrr_test2(Gtpoolrr *volatile pool, Greent *volatile green_thread, void *volatile varg) {
    //MT_TRACE();

    struct gtpoolrr_test_arg2 *volatile arg;
    volatile int fd;
    char *volatile buf;
    const char *volatile filename;
    if(pool == NULL || green_thread == NULL || varg == NULL) {
        goto gtpoolrr_test2_end;
    }

    arg = (struct gtpoolrr_test_arg2 *) varg;
    buf = arg->buf;
    filename = arg->filename;

    {
        mode_t mode = 0600;
        greent_do_open(green_thread, filename, O_RDONLY, mode);
        fd = green_thread->completion.res;
        if(fd <= 0) {
            printf("Error opening: %s\n", filename);
            goto gtpoolrr_test2_end;
        }
    }

    {
        greent_do_readt(green_thread, fd, buf, 256 - 1, 0, 1, 0);
        const int read_res = green_thread->completion.res;
        if(read_res < 0) {
            errno = -read_res;
            printf("Error reading %s:", filename);
            perror("");
        }
        buf[green_thread->completion.res] = 0;
    }

    MT_LOG("%s: %s", filename, buf);

gtpoolrr_test2_end:
    if(fd > 0) {
        greent_do_close(green_thread, fd);
        if(green_thread->completion.res != 0) {
            // should never happen
            assert(false);
        }
    }
    return NULL;
}

struct gtpoolrr_test_arg3 {
    char *buf;
    const char *in_filename;
    const char *out_filename;
};

// want to make sure greent_do_readt and greent_do_write work together
void *gtpoolrr_test3(Gtpoolrr *volatile pool, Greent *volatile green_thread, void *volatile varg) {
    if(pool == NULL || green_thread == NULL || varg == NULL) {
        return NULL;
    }

    volatile int in_fd = 0;
    volatile int out_fd = 0;
    struct gtpoolrr_test_arg3 *volatile arg = (struct gtpoolrr_test_arg3 *) varg;
    char *volatile buf = arg->buf;
    const char *volatile in_filename = arg->in_filename;
    const char *volatile out_filename = arg->out_filename;

    {
        mode_t mode = 0600;
        MT_LOG("start open file for reading");
        greent_do_open(green_thread, in_filename, O_RDONLY, mode);
        MT_LOG("end open file for reading");
        in_fd = green_thread->completion.res;
        if(in_fd <= 0) {
            printf("Error opening: %s\n", in_filename);
            assert(false);
            goto gtpoolrr_test3_end;
        }
    }

    {
        mode_t mode = 0600;
        MT_LOG("start open file for writing");
        greent_do_open(green_thread, out_filename, O_WRONLY | O_CREAT | O_TRUNC, mode);
        MT_LOG("end open file for writing");
        out_fd = green_thread->completion.res;
        if(out_fd <= 0) {
            printf("Error opening: %s\n", out_filename);
            assert(false);
            goto gtpoolrr_test3_end;
        }
    }

    {
        MT_LOG("start read file");
        greent_do_read(green_thread, in_fd, buf, 256 - 1, 0);
        MT_LOG("end read file");
        int io_res = green_thread->completion.res;
        if(io_res < 0) {
            errno = -io_res;
            printf("Error reading %s:", in_filename);
            perror("");
            assert(false);
            goto gtpoolrr_test3_end;
        }
        buf[green_thread->completion.res] = 0;

        MT_LOG("start write file");
        greent_do_write(green_thread, out_fd, buf, io_res, 0);
        MT_LOG("end write file");
        io_res = green_thread->completion.res;
        if(io_res < 0) {
            errno = -io_res;
            printf("Error writing %s:", out_filename);
            perror("");
            assert(false);
            goto gtpoolrr_test3_end;
        }
    }

gtpoolrr_test3_end:
    if(in_fd > 0) {
        MT_LOG("start close read file");
        greent_do_close(green_thread, in_fd);
        MT_LOG("end close read file");
        if(green_thread->completion.res != 0) {
            // should never happen
            assert(false);
        }
    }

    if(out_fd > 0) {
        MT_LOG("start close written file");
        greent_do_close(green_thread, out_fd);
        MT_LOG("end close written file");
        if(green_thread->completion.res != 0) {
            // should never happen
            assert(false);
        }
    }
    return NULL;
}

//int gtpoolrr_test(void) {
//MT_TRACE();

TEST(gtpoolrr, 1) {
    struct gtpoolrr_job *jobs[10] = { 0 };
    size_t num_obtained;
    Gtpoolrr *pool1;
    pool1 = gtpoolrr_create(1,1);
    ASSERT_NE(nullptr, pool1);
    ASSERT_EQ(0, gtpoolrr_pause(pool1));
    ASSERT_EQ(0, gtpoolrr_resume(pool1));
    ASSERT_EQ(0, gtpoolrr_sbs_get(pool1, jobs, &num_obtained, 1));
    gtpoolrr_sbs_set_tag(jobs[0], 8080);
    gtpoolrr_sbs_set_function(jobs[0], gtpoolrr_test1);
    struct gtpoolrr_test_arg1 arg = { 55 };
    gtpoolrr_sbs_set_arg(jobs[0], &arg);
    //gtpoolrr_sbs_set_expiration(jobs[0], 0);
    ASSERT_EQ(0, gtpoolrr_sbs_push_direct(pool1, 0, jobs[0]));

    gtpoolrr_join(pool1);
    ASSERT_EQ(0, gtpoolrr_cps_popall(pool1, jobs, 1));
    gtpoolrr_cps_ack(pool1, &(jobs[0]), 1);
    gtpoolrr_destroy(pool1);
}

TEST(gtpoolrr, 2) {
    Gtpoolrr *pool2;
    size_t num_pushed;
    pool2 = gtpoolrr_create(2,2);
    ASSERT_NE(nullptr, pool2);

    char bufs[2][256];
    const char *filenames[2] = { "./tests/DIR.txt", "./obj/dir.txt"};
    struct gtpoolrr_test_arg2 args[2] = { 0 };
    args[0] = { bufs[0], filenames[0] };
    args[1] = { bufs[1], filenames[1] };

    struct gtpoolrr_job *jobs[10] = { 0 };
    ASSERT_EQ(0, gtpoolrr_sbs_get(pool2, jobs, &num_pushed, 2));
    gtpoolrr_sbs_set_tag(jobs[0], 111);
    gtpoolrr_sbs_set_tag(jobs[1], 222);
    gtpoolrr_sbs_set_functions(jobs, 2, gtpoolrr_test2);
    gtpoolrr_sbs_set_arg(jobs[0], &(args[0]));
    gtpoolrr_sbs_set_arg(jobs[1], &(args[1]));
    gtpoolrr_sbs_set_expirations(jobs, 2, 10 * (1LLU << 30)); // about 10 seconds
    //ASSERT_EQ(0, gtpoolrr_sbs_pushall(pool2, &num_pushed, 2, jobs));
    ASSERT_EQ(0, gtpoolrr_sbs_push_direct(pool2, 0, jobs[0]));
    ASSERT_EQ(0, gtpoolrr_sbs_push_direct(pool2, 1, jobs[1]));

    ASSERT_EQ(0, gtpoolrr_cps_popall(pool2, jobs, 2));
    gtpoolrr_cps_ack(pool2, &(jobs[0]), 2);
    gtpoolrr_destroy(pool2);
}

TEST(gtpoolrr, 3) {
    Gtpoolrr *pool3;
    size_t num_pushed;
    pool3 = gtpoolrr_create(1,3);
    ASSERT_NE(pool3, nullptr);
    char bufs[3][256];
    const char *in_filenames[3] = {
        "tests/gtpoolrr/readtwritet_reference.txt",
        "tests/gtpoolrr/readtwritet_reference.txt",
        "tests/gtpoolrr/readtwritet_reference.txt"
    };
    const char *out_filenames[3] = {
        "./tests/gtpoolrr/readtwritet_a_out.txt",
        "./tests/gtpoolrr/readtwritet_b_out.txt",
        "./tests/gtpoolrr/readtwritet_c_out.txt"
    };
    struct gtpoolrr_test_arg3 arg0 = { bufs[0], in_filenames[0], out_filenames[0] };
    struct gtpoolrr_test_arg3 arg1 = { bufs[1], in_filenames[1], out_filenames[1] };
    struct gtpoolrr_test_arg3 arg2 = { bufs[2], in_filenames[2], out_filenames[2] };

    struct gtpoolrr_job *jobs[3] = { 0 };
    ASSERT_EQ(0, gtpoolrr_sbs_get(pool3, jobs, &num_pushed, 3));
    gtpoolrr_sbs_set_tag(jobs[0], 1 * 1111);
    gtpoolrr_sbs_set_tag(jobs[1], 2 * 1111);
    gtpoolrr_sbs_set_tag(jobs[2], 3 * 1111);
    gtpoolrr_sbs_set_functions(jobs, 3, gtpoolrr_test3);
    gtpoolrr_sbs_set_arg(jobs[0], &arg0);
    gtpoolrr_sbs_set_arg(jobs[1], &arg1);
    gtpoolrr_sbs_set_arg(jobs[2], &arg2);

    ASSERT_EQ(0, gtpoolrr_sbs_pushall_direct(pool3, 0, &num_pushed, 3, jobs));
    //ASSERT_EQ(0, gtpoolrr_sbs_push_direct(pool3, 0, jobs[0]));
    //ASSERT_EQ(0, gtpoolrr_sbs_push_direct(pool3, 1, jobs[0]));
    //ASSERT_EQ(0, gtpoolrr_sbs_push_direct(pool3, 2, jobs[0]));
    ASSERT_EQ(0, gtpoolrr_cps_popall(pool3, jobs, 3));
    gtpoolrr_join(pool3);
    gtpoolrr_destroy(pool3);
}

//return 0;
//}
}
