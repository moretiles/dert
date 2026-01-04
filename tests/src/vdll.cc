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
    assert(dll != NULL);
    assert(vdll_grow(dll, TEST_VDLL_ARRAY_LEN) == 0);

    long array[TEST_VDLL_ARRAY_LEN];
    long tmp;
    for(size_t i = 0; i < TEST_VDLL_ARRAY_LEN; i++) {
        array[i] = rand();
        assert(vdll_set(dll, i, &(array[i])) == 0);
    }

    for(size_t i = 0; i < TEST_VDLL_ARRAY_LEN * 10; i++) {
        size_t pos = rand() % TEST_VDLL_ARRAY_LEN;
        array[pos] = rand();
        assert(vdll_set(dll, pos, &(array[pos])) == 0);
    }

    for(size_t i = 0; i < TEST_VDLL_ARRAY_LEN * 100; i++) {
        size_t pos = rand() % TEST_VDLL_ARRAY_LEN;
        assert(vdll_get(dll, pos, &tmp) == 0);
        assert(tmp == array[pos]);
    }

    assert(vdll_len(dll) == TEST_VDLL_ARRAY_LEN);
    assert(vdll_shrink(dll, 1 + (TEST_VDLL_ARRAY_LEN / 2)) == 0);
    vdll_destroy(dll);
}
}
