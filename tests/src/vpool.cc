//#include <vpool.h>

//#include <gtest/gtest.h>

namespace {
TEST(vpool, kind_static) {
    Vpool *longs;
    long *a, *b, *c, *d;

    longs = vpool_create(3, sizeof(long), VPOOL_KIND_STATIC);
    ASSERT_NE(nullptr, longs);

    a = (long *) vpool_alloc(longs);
    b = (long *) vpool_alloc(longs);
    c = (long *) vpool_alloc(longs);
    d = (long *) vpool_alloc(longs);
    ASSERT_NE(nullptr, a);
    ASSERT_NE(nullptr, b);
    ASSERT_NE(nullptr, c);
    ASSERT_EQ(nullptr, d);
    ASSERT_NE(a, b);
    ASSERT_NE(b, c);
    ASSERT_NE(a, c);

    *a = 1;
    *b = 2;
    *c = 3;

    vpool_dealloc(longs, a);
    vpool_dealloc(longs, b);
    vpool_dealloc(longs, c);

    a = (long *) vpool_alloc(longs);
    b = (long *) vpool_alloc(longs);
    c = (long *) vpool_alloc(longs);

    vpool_dealloc(longs, a);
    vpool_dealloc(longs, c);

    vpool_destroy(longs);
}

TEST(vpool, kind_guided)    {
    Vpool *longs;
    long *a, *b, *c, *d, *e;

    longs = vpool_create(1, sizeof(long), VPOOL_KIND_GUIDED);
    ASSERT_NE(nullptr, longs);

    a = (long *) vpool_alloc(longs);
    b = (long *) vpool_alloc(longs);
    ASSERT_NE(nullptr, a);
    ASSERT_EQ(nullptr, b);
    ASSERT_TRUE(vpool_full(longs));

    size_t memory_size = vpool_advise(2, sizeof(long));
    void *memory = calloc(1, memory_size);
    ASSERT_NE(nullptr, memory);
    ASSERT_EQ(0, vpool_guided_extend(longs, memory, memory_size));
    b = (long *) vpool_alloc(longs);
    c = (long *) vpool_alloc(longs);
    d = (long *) vpool_alloc(longs);
    ASSERT_NE(nullptr, b);
    ASSERT_NE(nullptr, c);
    ASSERT_EQ(nullptr, d);

    *a = 1;
    *b = 2;
    *c = 3;

    vpool_dealloc(longs, a);
    vpool_dealloc(longs, b);
    vpool_dealloc(longs, c);

    void *memory2 = calloc(1, memory_size);
    ASSERT_NE(nullptr, memory2);
    ASSERT_EQ(0, vpool_guided_extend(longs, memory2, memory_size));

    a = (long *) vpool_alloc(longs);
    b = (long *) vpool_alloc(longs);
    c = (long *) vpool_alloc(longs);
    d = (long *) vpool_alloc(longs);
    e = (long *) vpool_alloc(longs);
    ASSERT_NE(nullptr, a);
    ASSERT_NE(nullptr, b);
    ASSERT_NE(nullptr, c);
    ASSERT_NE(nullptr, d);
    ASSERT_NE(nullptr, e);

    vpool_dealloc(longs, a);
    vpool_dealloc(longs, c);

    vpool_destroy(longs);
    free(memory2);
    free(memory);
}

TEST(vpool, kind_dynamic) {
    Vpool *longs;
    long *a, *b, *c, *d, *e;

    longs = vpool_create(1, sizeof(long), VPOOL_KIND_DYNAMIC);
    ASSERT_NE(nullptr, longs);

    a = (long *) vpool_alloc(longs);
    b = (long *) vpool_alloc(longs);
    c = (long *) vpool_alloc(longs);
    d = (long *) vpool_alloc(longs);
    e = (long *) vpool_alloc(longs);
    ASSERT_NE(nullptr, a);
    ASSERT_NE(nullptr, b);
    ASSERT_NE(nullptr, c);
    ASSERT_NE(nullptr, d);
    ASSERT_NE(nullptr, e);

    *a = 1;
    *b = 2;
    *c = 3;
    *d = 4;
    *e = 5;

    vpool_dealloc(longs, a);
    vpool_dealloc(longs, b);
    vpool_dealloc(longs, c);
    vpool_dealloc(longs, d);
    vpool_dealloc(longs, e);

    a = (long *) vpool_alloc(longs);
    b = (long *) vpool_alloc(longs);
    c = (long *) vpool_alloc(longs);
    d = (long *) vpool_alloc(longs);
    e = (long *) vpool_alloc(longs);
    ASSERT_NE(nullptr, a);
    ASSERT_NE(nullptr, b);
    ASSERT_NE(nullptr, c);
    ASSERT_NE(nullptr, d);
    ASSERT_NE(nullptr, e);

    vpool_dealloc(longs, a);
    vpool_dealloc(longs, c);

    vpool_destroy(longs);
}
}
