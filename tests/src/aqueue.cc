//#include <aqueue.h>

//#include <gtest/gtest.h>

namespace {
TEST(aqueue, init) {
    Aqueue *queue = aqueue_create(sizeof(long), 3);
    assert(queue != NULL);
    assert(aqueue_len(queue) == 0);
    assert(aqueue_cap(queue) == 3);

    long a = 1, b = 2, c = 3;
    assert(aqueue_enqueue(queue, &a) == 0);
    assert(aqueue_enqueue(queue, &b) == 0);
    assert(aqueue_enqueue(queue, &c) == 0);
    assert(aqueue_len(queue) == 3);
    assert(aqueue_cap(queue) == 3);

    long a_test, b_test, c_test;
    assert(aqueue_front(queue, &a_test) == 0);
    assert(a_test == a);
    assert(aqueue_dequeue(queue, &a_test) == 0);
    assert(aqueue_dequeue(queue, &b_test) == 0);
    assert(aqueue_dequeue(queue, &c_test) == 0);
    assert(a_test == a);
    assert(b_test == b);
    assert(c_test == c);

    assert(aqueue_len(queue) == 0);
    assert(aqueue_cap(queue) == 3);
    aqueue_destroy(queue);
}

TEST(aqueue, initv) {
    Aqueue *queue = aqueue_create(sizeof(long), 3);
    size_t num_enqueued = 0;
    size_t num_dequeued = 0;
    assert(queue != NULL);
    assert(aqueue_len(queue) == 0);
    assert(aqueue_cap(queue) == 3);

    long two_longs_src[2] = { 1, 2 };
    long two_longs_dest[2] = { 0, 0 };
    long two_more_longs_src[2] = { 3, 4 };
    long two_more_longs_dest[2] = { 0, 67 };
    assert(aqueue_enqueue_some(queue, &num_enqueued, 2, two_longs_src) == 0);
    assert(num_enqueued == 2);
    assert(aqueue_dequeue_some(queue, &num_dequeued, 2, two_longs_dest) == 0);
    assert(num_dequeued == 2);
    assert(two_longs_dest[0] == 1);
    assert(two_longs_dest[1] == 2);
    assert(aqueue_enqueue_some(queue, &num_enqueued, 2, two_longs_src) == 0);
    assert(num_enqueued == 2);
    assert(aqueue_enqueue_some(queue, &num_enqueued, 2, two_more_longs_src) == EXFULL);
    assert(num_enqueued == 1);
    assert(aqueue_dequeue_some(queue, &num_dequeued, 2, two_longs_dest) == 0);
    assert(num_dequeued == 2);
    assert(two_longs_dest[0] == 1);
    assert(two_longs_dest[1] == 2);
    assert(aqueue_dequeue_some(queue, &num_dequeued, 2, two_more_longs_dest) == ENODATA);
    assert(num_dequeued == 1);
    assert(two_more_longs_dest[0] == 3);
    assert(two_more_longs_dest[1] == 67);

    free(queue);
}
}
