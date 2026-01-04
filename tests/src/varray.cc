//#include <varray.h>

//#include <gtest/gtest.h>

namespace {
TEST(varray, simple) {
    Varray *array = varray_create(sizeof(long));
    ASSERT_NE(nullptr, array);
    ASSERT_EQ(0, varray_len(array));
    ASSERT_EQ(0, varray_cap(array));
    ASSERT_EQ(0, varray_realloc(array, 4));
    ASSERT_EQ(4, varray_len(array));
    ASSERT_GE(4, varray_cap(array));
    ASSERT_EQ(0, varray_realloc(array, 5));
    ASSERT_EQ(5, varray_len(array));
    ASSERT_GE(5, varray_cap(array));

    long a = 1, b = 2, c = 3;
    ASSERT_EQ(0, varray_set(array, 0, &a));
    ASSERT_EQ(0, varray_set(array, 1, &b));
    ASSERT_EQ(0, varray_set(array, 2, &c));

    long a_test, b_test, c_test;
    ASSERT_EQ(0, varray_get(array, 0, &a_test));
    ASSERT_EQ(0, varray_get(array, 1, &b_test));
    ASSERT_EQ(0, varray_get(array, 2, &c_test));
    ASSERT_EQ(a_test, a);
    ASSERT_EQ(b_test, b);
    ASSERT_EQ(c_test, c);

    ASSERT_EQ(0, varray_realloc(array, 4));
    ASSERT_EQ(4, varray_len(array));
    ASSERT_GE(varray_cap(array), 4);

    ASSERT_EQ(0, varray_resize(array, 0));
    ASSERT_EQ(0, varray_len(array));
    ASSERT_GE(varray_cap(array), 3);
    ASSERT_NE(0, varray_get(array, 0, &a_test) );
    ASSERT_NE(0, varray_get(array, 1, &b_test) );
    ASSERT_NE(0, varray_get(array, 2, &c_test) );
    ASSERT_EQ(0, varray_resize(array, 3));
    ASSERT_EQ(3, varray_len(array));
    ASSERT_GE(varray_cap(array), 3);
    ASSERT_EQ(0, varray_get(array, 0, &a_test));
    ASSERT_EQ(0, varray_get(array, 1, &b_test));
    ASSERT_EQ(0, varray_get(array, 2, &c_test));
    ASSERT_EQ(a, a_test);
    ASSERT_EQ(b, b_test);
    ASSERT_EQ(c, c_test);

    varray_destroy(array);
}
}
