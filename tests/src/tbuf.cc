//#include <tbuf.h>

//#include <gtest/gtest.h>

//#include <sys/random.h>

namespace {
TEST(tbuf, init) {
#define TBUF_TEST_BUF_SIZE (99)
    const char *TBUF_TEST_STR_A = "0123456789";
    const char * TBUF_TEST_STR_B = "ABCDEF";

    char *a, *b;
    Tbuf *twin = tbuf_create(TBUF_TEST_BUF_SIZE);
    ASSERT_NE(nullptr, twin);
    ASSERT_EQ(TBUF_TEST_BUF_SIZE, tbuf_cap(twin));

    a = twin->A;
    b = twin->B;
    ASSERT_NE(nullptr, a);
    ASSERT_NE(nullptr, b);
    ASSERT_NE(nullptr, strcpy(a, TBUF_TEST_STR_A));
    twin->A_len += strlen(TBUF_TEST_STR_A) + 1;
    twin->B_len += strlen(TBUF_TEST_STR_B) + 1;
    ASSERT_NE(nullptr, strcpy(b, TBUF_TEST_STR_B));
    ASSERT_EQ(tbuf_A_unused(twin), TBUF_TEST_BUF_SIZE - strlen(TBUF_TEST_STR_A) - 1);
    ASSERT_EQ(tbuf_B_unused(twin), TBUF_TEST_BUF_SIZE - strlen(TBUF_TEST_STR_B) - 1);
    ASSERT_STREQ(a, TBUF_TEST_STR_A);
    ASSERT_STREQ(b, TBUF_TEST_STR_B);

    ASSERT_EQ(0, tbuf_swap(twin));
    a = twin->A;
    b = twin->B;
    ASSERT_NE(nullptr, a);
    ASSERT_NE(nullptr, b);
    ASSERT_STREQ(a, TBUF_TEST_STR_B);
    ASSERT_STREQ(b, TBUF_TEST_STR_A);

    a = twin->A;
    b = twin->B;
    ASSERT_NE(nullptr, a);
    ASSERT_NE(nullptr, b);
    ASSERT_STREQ(a, TBUF_TEST_STR_B);
    ASSERT_STREQ(b, TBUF_TEST_STR_A);
    ASSERT_EQ(0, tbuf_swap(twin));
    a = twin->A;
    b = twin->B;
    ASSERT_NE(nullptr, a);
    ASSERT_NE(nullptr, b);

    tbuf_destroy(twin);
}

TEST(tbuf, initv) {
    char *a, *b, *A, *B;
#define TBUF_TEST_NUM_TWINS (10)
    Tbuf *twins;
    void *memory = calloc(1, tbuf_advisev(TBUF_TEST_NUM_TWINS, TBUF_TEST_BUF_SIZE));
    ASSERT_NE(nullptr, memory);
    ASSERT_EQ(0, tbuf_initv(TBUF_TEST_NUM_TWINS, &twins, memory, TBUF_TEST_BUF_SIZE));

    for(size_t i = 0; i < TBUF_TEST_NUM_TWINS; i++) {
        a = tbuf_A(&(twins[i]));
        b = tbuf_B(&(twins[i]));
        ASSERT_EQ(16, getrandom(a, 16, 0));
        ASSERT_EQ(16, getrandom(b, 16, 0));
        A = a;
        B = b;
        ASSERT_EQ(0, tbuf_swap(&(twins[i])));
        a = tbuf_A(&(twins[i]));
        b = tbuf_B(&(twins[i]));
        ASSERT_STREQ(a, B);
        ASSERT_STREQ(b, A);
        tbuf_deinit(&(twins[i]));
    }
    free(&(twins[0]));
}
}
