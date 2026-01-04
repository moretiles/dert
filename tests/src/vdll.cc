//#include <vdll.h>

//#include <gtest/gtest.h>

namespace {
int init_long(void *ptr) {
    if(ptr == NULL) {
        return 1;
    }

    long *long_ptr = (long *) ptr;
    *long_ptr = 1;
    return 0;
}

int deinit_long(void *ptr) {
    if(ptr == NULL) {
        return 1;
    }

    long *long_ptr = (long *) ptr;
    *long_ptr = 0;
    return 0;
}

TEST(vdll, test) {
    Vdll_functions functions;
    functions.init = init_long;
    functions.deinit = deinit_long;
#define TEST_VDLL_ARRAY_LEN (99)
    Vdll *dll = vdll_create(sizeof(long), &functions);
    ASSERT_NE(nullptr, dll);
    ASSERT_EQ(0, vdll_grow(dll, TEST_VDLL_ARRAY_LEN));

    long array[TEST_VDLL_ARRAY_LEN];
    long tmp;
    for(size_t i = 0; i < TEST_VDLL_ARRAY_LEN; i++) {
        array[i] = rand();
        ASSERT_EQ(0, vdll_set(dll, i, &(array[i])));
    }

    for(size_t i = 0; i < TEST_VDLL_ARRAY_LEN * 10; i++) {
        size_t pos = rand() % TEST_VDLL_ARRAY_LEN;
        array[pos] = rand();
        ASSERT_EQ(0, vdll_set(dll, pos, &(array[pos])));
    }

    for(size_t i = 0; i < TEST_VDLL_ARRAY_LEN * 100; i++) {
        size_t pos = rand() % TEST_VDLL_ARRAY_LEN;
        ASSERT_EQ(0, vdll_get(dll, pos, &tmp));
        ASSERT_EQ(tmp, array[pos]);
    }

    ASSERT_EQ(TEST_VDLL_ARRAY_LEN, vdll_len(dll));
    ASSERT_EQ(0, vdll_shrink(dll, 1 + (TEST_VDLL_ARRAY_LEN / 2)));
    vdll_destroy(dll);
}
}
