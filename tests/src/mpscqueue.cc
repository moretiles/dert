//#include <mpscqueue.h>

//#include <gtest/gtest.h>

namespace {
TEST(mpscqueue, nooverwrite) {
    Mpscqueue *queue = mpscqueue_create(sizeof(long), 3);
    ASSERT_NE(nullptr, queue);
    ASSERT_EQ(0, mpscqueue_len(queue));
    ASSERT_EQ(3, mpscqueue_cap(queue));

    long a = 1, b = 2, c = 3;
    ASSERT_EQ(0, mpscqueue_enqueue(queue, &a));
    ASSERT_EQ(0, mpscqueue_enqueue(queue, &b));
    ASSERT_EQ(0, mpscqueue_enqueue(queue, &c));
    ASSERT_EQ(3, mpscqueue_len(queue));
    ASSERT_EQ(3, mpscqueue_cap(queue));

    long a_test, b_test, c_test;
    ASSERT_EQ(0, mpscqueue_front(queue, &a_test));
    ASSERT_EQ(a_test, a);
    ASSERT_EQ(0, mpscqueue_dequeue(queue, &a_test));
    ASSERT_EQ(0, mpscqueue_dequeue(queue, &b_test));
    ASSERT_EQ(0, mpscqueue_dequeue(queue, &c_test));
    ASSERT_EQ(a_test, a);
    ASSERT_EQ(b_test, b);
    ASSERT_EQ(c_test, c);

    ASSERT_EQ(0, mpscqueue_len(queue));
    ASSERT_EQ(3, mpscqueue_cap(queue));
    mpscqueue_destroy(queue);
}
}
