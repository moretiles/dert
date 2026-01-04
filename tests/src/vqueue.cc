//#include <vqueue.h>

//#include <gtest/gtest.h>

namespace {
TEST(vqueue, nooverwrite) {
    Vqueue *queue = vqueue_create(sizeof(long), 3);
    ASSERT_NE(nullptr, queue);
    ASSERT_EQ(0, vqueue_len(queue));
    ASSERT_EQ(3, vqueue_cap(queue));

    long a = 1, b = 2, c = 3;
    ASSERT_EQ(0, vqueue_enqueue(queue, &a, false));
    ASSERT_EQ(0, vqueue_enqueue(queue, &b, false));
    ASSERT_EQ(0, vqueue_enqueue(queue, &c, false));
    ASSERT_EQ(3, vqueue_len(queue));
    ASSERT_EQ(3, vqueue_cap(queue));

    long a_test, b_test, c_test;
    ASSERT_EQ(0, vqueue_front(queue, &a_test));
    ASSERT_EQ(0, vqueue_back(queue, &c_test));
    ASSERT_EQ(a, a_test);
    ASSERT_EQ(c, c_test);
    ASSERT_EQ(0, vqueue_dequeue(queue, &a_test));
    ASSERT_EQ(0, vqueue_dequeue(queue, &b_test));
    ASSERT_EQ(0, vqueue_dequeue(queue, &c_test));
    ASSERT_EQ(a, a_test);
    ASSERT_EQ(b, b_test);
    ASSERT_EQ(c, c_test);

    ASSERT_EQ(0, vqueue_len(queue));
    ASSERT_EQ(3, vqueue_cap(queue));
    vqueue_destroy(queue);
}

TEST(vqueue, overwrite) {
    Vqueue *queue = vqueue_create(sizeof(long), 3);
    ASSERT_NE(nullptr, queue);
    ASSERT_EQ(0, vqueue_len(queue));
    ASSERT_EQ(3, vqueue_cap(queue));

    long a = 1, b = 2, c = 3, d = 4;
    ASSERT_EQ(0, vqueue_enqueue(queue, &a, true));
    ASSERT_EQ(0, vqueue_enqueue(queue, &b, true));
    ASSERT_EQ(0, vqueue_enqueue(queue, &c, true));
    ASSERT_EQ(0, vqueue_enqueue(queue, &d, true));
    ASSERT_EQ(3, vqueue_len(queue));
    ASSERT_EQ(3, vqueue_cap(queue));

    long b_test, c_test, d_test;
    ASSERT_EQ(0, vqueue_front(queue, &b_test));
    ASSERT_EQ(0, vqueue_back(queue, &d_test));
    ASSERT_EQ(b, b_test);
    ASSERT_EQ(d, d_test);
    ASSERT_EQ(0, vqueue_dequeue(queue, &b_test));
    ASSERT_EQ(0, vqueue_dequeue(queue, &c_test));
    ASSERT_EQ(b, b_test);
    ASSERT_EQ(c, c_test);
    ASSERT_EQ(1, vqueue_len(queue));
    ASSERT_EQ(3, vqueue_cap(queue));

    long e = 5, f = 6;
    ASSERT_EQ(0, vqueue_enqueue(queue, &e, true));
    ASSERT_EQ(0, vqueue_enqueue(queue, &f, true));
    ASSERT_EQ(3, vqueue_len(queue));
    ASSERT_EQ(3, vqueue_cap(queue));

    long e_test, f_test;
    ASSERT_EQ(0, vqueue_front(queue, &d_test));
    ASSERT_EQ(0, vqueue_back(queue, &f_test));
    ASSERT_EQ(d, d_test);
    ASSERT_EQ(f, f_test);
    ASSERT_EQ(0, vqueue_dequeue(queue, &d_test));
    ASSERT_EQ(0, vqueue_dequeue(queue, &e_test));
    ASSERT_EQ(0, vqueue_dequeue(queue, &f_test));
    ASSERT_EQ(d, d_test);
    ASSERT_EQ(e, e_test);
    ASSERT_EQ(f, f_test);

    ASSERT_EQ(0, vqueue_len(queue));
    ASSERT_EQ(3, vqueue_cap(queue));
    vqueue_destroy(queue);
}

TEST(vqueue, some_nooverwrite) {
    Vqueue *queue = vqueue_create(sizeof(long), 3);
    size_t num_enqueued = 0;
    size_t num_dequeued = 0;
    ASSERT_NE(nullptr, queue);
    ASSERT_EQ(0, vqueue_len(queue));
    ASSERT_EQ(3, vqueue_cap(queue));

    long two_longs_src[2] = { 1, 2 };
    long two_longs_dest[2] = { 0, 0 };
    long two_more_longs_src[2] = { 3, 4 };
    long two_more_longs_dest[2] = { 0, 67 };
    ASSERT_EQ(0, vqueue_enqueue_some(queue, &num_enqueued, 2, two_longs_src, false));
    ASSERT_EQ(2, num_enqueued);
    ASSERT_EQ(0, vqueue_dequeue_some(queue, &num_dequeued, 2, two_longs_dest));
    ASSERT_EQ(2, num_dequeued);
    ASSERT_EQ(1, two_longs_dest[0]);
    ASSERT_EQ(2, two_longs_dest[1]);
    ASSERT_EQ(0, vqueue_enqueue_some(queue, &num_enqueued, 2, two_longs_src, false));
    ASSERT_EQ(2, num_enqueued);
    ASSERT_EQ(EXFULL, vqueue_enqueue_some(queue, &num_enqueued, 2, two_more_longs_src, false));
    ASSERT_EQ(1, num_enqueued);
    ASSERT_EQ(0, vqueue_dequeue_some(queue, &num_dequeued, 2, two_longs_dest));
    ASSERT_EQ(2, num_dequeued);
    ASSERT_EQ(1, two_longs_dest[0]);
    ASSERT_EQ(2, two_longs_dest[1]);
    ASSERT_EQ(ENODATA, vqueue_dequeue_some(queue, &num_dequeued, 2, two_more_longs_dest));
    ASSERT_EQ(1, num_dequeued);
    ASSERT_EQ(3, two_more_longs_dest[0]);
    ASSERT_EQ(67, two_more_longs_dest[1]);

    free(queue);
}

TEST(vqueue, some_overwrite) {
    Vqueue *queue = vqueue_create(sizeof(long), 3);
    size_t num_enqueued = 0;
    size_t num_dequeued = 0;
    ASSERT_NE(nullptr, queue);
    ASSERT_EQ(0, vqueue_len(queue));
    ASSERT_EQ(3, vqueue_cap(queue));

    long two_longs_src[2] = { 1, 2 };
    long two_longs_dest[2] = { 0, 0 };
    long two_more_longs_src[2] = { 3, 4 };
    long two_more_longs_dest[2] = { 0, 67 };
    ASSERT_EQ(0, vqueue_enqueue_some(queue, &num_enqueued, 2, two_longs_src, true));
    ASSERT_EQ(2, num_enqueued);
    ASSERT_EQ(0, vqueue_dequeue_some(queue, &num_dequeued, 2, two_longs_dest));
    ASSERT_EQ(2, num_dequeued);
    ASSERT_EQ(1, two_longs_dest[0]);
    ASSERT_EQ(2, two_longs_dest[1]);
    ASSERT_EQ(0, vqueue_enqueue_some(queue, &num_enqueued, 2, two_longs_src, true));
    ASSERT_EQ(2, num_enqueued);
    ASSERT_EQ(0, vqueue_enqueue_some(queue, &num_enqueued, 2, two_more_longs_src, true));
    ASSERT_EQ(2, num_enqueued);
    ASSERT_EQ(0, vqueue_dequeue_some(queue, &num_dequeued, 2, two_longs_dest));
    ASSERT_EQ(2, num_dequeued);
    ASSERT_EQ(2, two_longs_dest[0]);
    ASSERT_EQ(3, two_longs_dest[1]);
    ASSERT_EQ(ENODATA, vqueue_dequeue_some(queue, &num_dequeued, 2, two_more_longs_dest));
    ASSERT_EQ(1, num_dequeued);
    ASSERT_EQ(4, two_more_longs_dest[0]);
    ASSERT_EQ(67, two_more_longs_dest[1]);

    free(queue);
}
}
