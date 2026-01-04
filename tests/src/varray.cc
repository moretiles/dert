//#include <varray.h>

//#include <gtest/gtest.h>

namespace {
TEST(varray, simple) {
    Varray *array = varray_create(sizeof(long));
    assert(array != NULL);
    assert(varray_len(array) == 0);
    assert(varray_cap(array) == 0);
    assert(varray_realloc(array, 4) == 0);
    assert(varray_len(array) == 4);
    assert(varray_cap(array) >= 4);
    assert(varray_realloc(array, 5) == 0);
    assert(varray_len(array) == 5);
    assert(varray_cap(array) >= 5);

    long a = 1, b = 2, c = 3;
    assert(varray_set(array, 0, &a) == 0);
    assert(varray_set(array, 1, &b) == 0);
    assert(varray_set(array, 2, &c) == 0);

    long a_test, b_test, c_test;
    assert(varray_get(array, 0, &a_test) == 0);
    assert(varray_get(array, 1, &b_test) == 0);
    assert(varray_get(array, 2, &c_test) == 0);
    assert(a_test == a);
    assert(b_test == b);
    assert(c_test == c);

    assert(varray_realloc(array, 4) == 0);
    assert(varray_len(array) == 4);
    assert(varray_cap(array) >= 4);

    assert(varray_resize(array, 0) == 0);
    assert(varray_len(array) == 0);
    assert(varray_cap(array) >= 3);
    assert(varray_get(array, 0, &a_test) != 0);
    assert(varray_get(array, 1, &b_test) != 0);
    assert(varray_get(array, 2, &c_test) != 0);
    assert(varray_resize(array, 3) == 0);
    assert(varray_len(array) == 3);
    assert(varray_cap(array) >= 3);
    assert(varray_get(array, 0, &a_test) == 0);
    assert(varray_get(array, 1, &b_test) == 0);
    assert(varray_get(array, 2, &c_test) == 0);
    assert(a_test == a);
    assert(b_test == b);
    assert(c_test == c);

    varray_destroy(array);
}
}
