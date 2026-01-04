//#include <aqueue.h>

//#include <gtest/gtest.h>

namespace {
TEST(aqueue, init) {
    Aqueue *queue = aqueue_create(sizeof(long), 3);
    ASSERT_NE(queue, nullptr);
    ASSERT_EQ(0, aqueue_len(queue));
    ASSERT_EQ(3, aqueue_cap(queue));

    long a = 1, b = 2, c = 3;
    ASSERT_EQ(0, aqueue_enqueue(queue, &a));
    ASSERT_EQ(0, aqueue_enqueue(queue, &b));
    ASSERT_EQ(0, aqueue_enqueue(queue, &c));
    ASSERT_EQ(3, aqueue_len(queue));
    ASSERT_EQ(3, aqueue_cap(queue));

    long a_test, b_test, c_test;
    ASSERT_EQ(0, aqueue_front(queue, &a_test));
    ASSERT_EQ(a_test, a);
    ASSERT_EQ(0, aqueue_dequeue(queue, &a_test));
    ASSERT_EQ(0, aqueue_dequeue(queue, &b_test));
    ASSERT_EQ(0, aqueue_dequeue(queue, &c_test));
    ASSERT_EQ(a_test, a);
    ASSERT_EQ(b_test, b);
    ASSERT_EQ(c_test, c);

    ASSERT_EQ(0, aqueue_len(queue));
    ASSERT_EQ(3, aqueue_cap(queue));
    aqueue_destroy(queue);
}

TEST(aqueue, initv) {
    Aqueue *queue = aqueue_create(sizeof(long), 3);
    size_t num_enqueued = 0;
    size_t num_dequeued = 0;
    ASSERT_NE(queue, nullptr);
    ASSERT_EQ(0, aqueue_len(queue));
    ASSERT_EQ(3, aqueue_cap(queue));

    long two_longs_src[2] = { 1, 2 };
    long two_longs_dest[2] = { 0, 0 };
    long two_more_longs_src[2] = { 3, 4 };
    long two_more_longs_dest[2] = { 0, 67 };
    ASSERT_EQ(0, aqueue_enqueue_some(queue, &num_enqueued, 2, two_longs_src));
    ASSERT_EQ(2, num_enqueued);
    ASSERT_EQ(0, aqueue_dequeue_some(queue, &num_dequeued, 2, two_longs_dest));
    ASSERT_EQ(2, num_dequeued);
    ASSERT_EQ(1, two_longs_dest[0]);
    ASSERT_EQ(2, two_longs_dest[1]);
    ASSERT_EQ(0, aqueue_enqueue_some(queue, &num_enqueued, 2, two_longs_src));
    ASSERT_EQ(2, num_enqueued);
    ASSERT_EQ(EXFULL, aqueue_enqueue_some(queue, &num_enqueued, 2, two_more_longs_src));
    ASSERT_EQ(1, num_enqueued);
    ASSERT_EQ(0, aqueue_dequeue_some(queue, &num_dequeued, 2, two_longs_dest));
    ASSERT_EQ(2, num_dequeued);
    ASSERT_EQ(1, two_longs_dest[0]);
    ASSERT_EQ(2, two_longs_dest[1]);
    ASSERT_EQ(ENODATA, aqueue_dequeue_some(queue, &num_dequeued, 2, two_more_longs_dest));
    ASSERT_EQ(1, num_dequeued);
    ASSERT_EQ(3, two_more_longs_dest[0]);
    ASSERT_EQ(67, two_more_longs_dest[1]);

    free(queue);
}
}
