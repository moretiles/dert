//#include <mpscqueue.h>

//#include <gtest/gtest.h>

namespace {
TEST(mpscqueue, nooverwrite) {
    Mpscqueue *queue = mpscqueue_create(sizeof(long), 3);
    assert(queue != NULL);
    assert(mpscqueue_len(queue) == 0);
    assert(mpscqueue_cap(queue) == 3);

    long a = 1, b = 2, c = 3;
    assert(mpscqueue_enqueue(queue, &a) == 0);
    assert(mpscqueue_enqueue(queue, &b) == 0);
    assert(mpscqueue_enqueue(queue, &c) == 0);
    assert(mpscqueue_len(queue) == 3);
    assert(mpscqueue_cap(queue) == 3);

    long a_test, b_test, c_test;
    assert(mpscqueue_front(queue, &a_test) == 0);
    assert(a_test == a);
    assert(mpscqueue_dequeue(queue, &a_test) == 0);
    assert(mpscqueue_dequeue(queue, &b_test) == 0);
    assert(mpscqueue_dequeue(queue, &c_test) == 0);
    assert(a_test == a);
    assert(b_test == b);
    assert(c_test == c);

    assert(mpscqueue_len(queue) == 0);
    assert(mpscqueue_cap(queue) == 3);
    mpscqueue_destroy(queue);
}
}
