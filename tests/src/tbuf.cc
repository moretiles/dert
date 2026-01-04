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
    assert(tbuf_cap(twin) == TBUF_TEST_BUF_SIZE);

    assert(twin != NULL);
    assert(tbuf_cap(twin) == TBUF_TEST_BUF_SIZE);
    a = twin->A;
    b = twin->B;
    assert(a != NULL);
    assert(b != NULL);
    assert(strcpy(a, TBUF_TEST_STR_A) != NULL);
    twin->A_len += strlen(TBUF_TEST_STR_A) + 1;
    twin->B_len += strlen(TBUF_TEST_STR_B) + 1;
    assert(strcpy(b, TBUF_TEST_STR_B) != NULL);
    assert(tbuf_A_unused(twin) == TBUF_TEST_BUF_SIZE - strlen(TBUF_TEST_STR_A) - 1);
    assert(tbuf_B_unused(twin) == TBUF_TEST_BUF_SIZE - strlen(TBUF_TEST_STR_B) - 1);
    assert(!strcmp(a, TBUF_TEST_STR_A));
    assert(!strcmp(b, TBUF_TEST_STR_B));

    assert(tbuf_swap(twin) == 0);
    a = twin->A;
    b = twin->B;
    assert(a != NULL);
    assert(b != NULL);
    assert(!strcmp(a, TBUF_TEST_STR_B));
    assert(!strcmp(b, TBUF_TEST_STR_A));

    a = twin->A;
    b = twin->B;
    assert(a != NULL);
    assert(b != NULL);
    assert(!strcmp(a, TBUF_TEST_STR_B));
    assert(!strcmp(b, TBUF_TEST_STR_A));
    assert(tbuf_swap(twin) == 0);
    a = twin->A;
    b = twin->B;
    assert(a != NULL);
    assert(b != NULL);
    assert(!strcmp(a, TBUF_TEST_STR_A));
    assert(!strcmp(b, TBUF_TEST_STR_B));

    tbuf_destroy(twin);
}

TEST(tbuf, initv) {
    char *a, *b, *A, *B;
#define TBUF_TEST_NUM_TWINS (10)
    Tbuf *twins;
    void *memory = calloc(1, tbuf_advisev(TBUF_TEST_NUM_TWINS, TBUF_TEST_BUF_SIZE));
    assert(memory != NULL);
    assert(tbuf_initv(TBUF_TEST_NUM_TWINS, &twins, memory, TBUF_TEST_BUF_SIZE) == 0);

    for(size_t i = 0; i < TBUF_TEST_NUM_TWINS; i++) {
        a = tbuf_A(&(twins[i]));
        b = tbuf_B(&(twins[i]));
        assert(getrandom(a, 16, 0) == 16);
        assert(getrandom(b, 16, 0) == 16);
        A = a;
        B = b;
        assert(tbuf_swap(&(twins[i])) == 0);
        a = tbuf_A(&(twins[i]));
        b = tbuf_B(&(twins[i]));
        assert(!strcmp(a, B));
        assert(!strcmp(b, A));
        tbuf_deinit(&(twins[i]));
    }
    free(&(twins[0]));
}
}
