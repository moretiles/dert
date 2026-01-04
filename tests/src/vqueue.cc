//#include <vqueue.h>

//#include <gtest/gtest.h>

namespace {
TEST(vqueue, nooverwrite) {
    Vqueue *queue = vqueue_create(sizeof(long), 3);
    assert(queue != NULL);
    assert(vqueue_len(queue) == 0);
    assert(vqueue_cap(queue) == 3);

    long a = 1, b = 2, c = 3;
    assert(vqueue_enqueue(queue, &a, false) == 0);
    assert(vqueue_enqueue(queue, &b, false) == 0);
    assert(vqueue_enqueue(queue, &c, false) == 0);
    assert(vqueue_len(queue) == 3);
    assert(vqueue_cap(queue) == 3);

    long a_test, b_test, c_test;
    assert(vqueue_front(queue, &a_test) == 0);
    assert(vqueue_back(queue, &c_test) == 0);
    assert(a_test == a);
    assert(c_test == c);
    assert(vqueue_dequeue(queue, &a_test) == 0);
    assert(vqueue_dequeue(queue, &b_test) == 0);
    assert(vqueue_dequeue(queue, &c_test) == 0);
    assert(a_test == a);
    assert(b_test == b);
    assert(c_test == c);

    assert(vqueue_len(queue) == 0);
    assert(vqueue_cap(queue) == 3);
    vqueue_destroy(queue);
}

TEST(vqueue, overwrite) {
    Vqueue *queue = vqueue_create(sizeof(long), 3);
    assert(queue != NULL);
    assert(vqueue_len(queue) == 0);
    assert(vqueue_cap(queue) == 3);

    long a = 1, b = 2, c = 3, d = 4;
    assert(vqueue_enqueue(queue, &a, true) == 0);
    assert(vqueue_enqueue(queue, &b, true) == 0);
    assert(vqueue_enqueue(queue, &c, true) == 0);
    assert(vqueue_enqueue(queue, &d, true) == 0);
    assert(vqueue_len(queue) == 3);
    assert(vqueue_cap(queue) == 3);

    long b_test, c_test, d_test;
    assert(vqueue_front(queue, &b_test) == 0);
    assert(vqueue_back(queue, &d_test) == 0);
    assert(b_test == b);
    assert(d_test == d);
    assert(vqueue_dequeue(queue, &b_test) == 0);
    assert(vqueue_dequeue(queue, &c_test) == 0);
    assert(b_test == b);
    assert(c_test == c);
    assert(vqueue_len(queue) == 1);
    assert(vqueue_cap(queue) == 3);

    long e = 5, f = 6;
    assert(vqueue_enqueue(queue, &e, true) == 0);
    assert(vqueue_enqueue(queue, &f, true) == 0);
    assert(vqueue_len(queue) == 3);
    assert(vqueue_cap(queue) == 3);

    long e_test, f_test;
    assert(vqueue_front(queue, &d_test) == 0);
    assert(vqueue_back(queue, &f_test) == 0);
    assert(d_test == d);
    assert(f_test == f);
    assert(vqueue_dequeue(queue, &d_test) == 0);
    assert(vqueue_dequeue(queue, &e_test) == 0);
    assert(vqueue_dequeue(queue, &f_test) == 0);
    assert(d_test == d);
    assert(e_test == e);
    assert(f_test == f);

    assert(vqueue_len(queue) == 0);
    assert(vqueue_cap(queue) == 3);
    vqueue_destroy(queue);
}

TEST(vqueue, some_nooverwrite) {
    Vqueue *queue = vqueue_create(sizeof(long), 3);
    size_t num_enqueued = 0;
    size_t num_dequeued = 0;
    assert(queue != NULL);
    assert(vqueue_len(queue) == 0);
    assert(vqueue_cap(queue) == 3);

    long two_longs_src[2] = { 1, 2 };
    long two_longs_dest[2] = { 0, 0 };
    long two_more_longs_src[2] = { 3, 4 };
    long two_more_longs_dest[2] = { 0, 67 };
    assert(vqueue_enqueue_some(queue, &num_enqueued, 2, two_longs_src, false) == 0);
    assert(num_enqueued == 2);
    assert(vqueue_dequeue_some(queue, &num_dequeued, 2, two_longs_dest) == 0);
    assert(num_dequeued == 2);
    assert(two_longs_dest[0] == 1);
    assert(two_longs_dest[1] == 2);
    assert(vqueue_enqueue_some(queue, &num_enqueued, 2, two_longs_src, false) == 0);
    assert(num_enqueued == 2);
    assert(vqueue_enqueue_some(queue, &num_enqueued, 2, two_more_longs_src, false) == EXFULL);
    assert(num_enqueued == 1);
    assert(vqueue_dequeue_some(queue, &num_dequeued, 2, two_longs_dest) == 0);
    assert(num_dequeued == 2);
    assert(two_longs_dest[0] == 1);
    assert(two_longs_dest[1] == 2);
    assert(vqueue_dequeue_some(queue, &num_dequeued, 2, two_more_longs_dest) == ENODATA);
    assert(num_dequeued == 1);
    assert(two_more_longs_dest[0] == 3);
    assert(two_more_longs_dest[1] == 67);

    free(queue);
}

TEST(vqueue, some_overwrite) {
    Vqueue *queue = vqueue_create(sizeof(long), 3);
    size_t num_enqueued = 0;
    size_t num_dequeued = 0;
    assert(queue != NULL);
    assert(vqueue_len(queue) == 0);
    assert(vqueue_cap(queue) == 3);

    long two_longs_src[2] = { 1, 2 };
    long two_longs_dest[2] = { 0, 0 };
    long two_more_longs_src[2] = { 3, 4 };
    long two_more_longs_dest[2] = { 0, 67 };
    assert(vqueue_enqueue_some(queue, &num_enqueued, 2, two_longs_src, true) == 0);
    assert(num_enqueued == 2);
    assert(vqueue_dequeue_some(queue, &num_dequeued, 2, two_longs_dest) == 0);
    assert(num_dequeued == 2);
    assert(two_longs_dest[0] == 1);
    assert(two_longs_dest[1] == 2);
    assert(vqueue_enqueue_some(queue, &num_enqueued, 2, two_longs_src, true) == 0);
    assert(num_enqueued == 2);
    assert(vqueue_enqueue_some(queue, &num_enqueued, 2, two_more_longs_src, true) == 0);
    assert(num_enqueued == 2);
    assert(vqueue_dequeue_some(queue, &num_dequeued, 2, two_longs_dest) == 0);
    assert(num_dequeued == 2);
    assert(two_longs_dest[0] == 2);
    assert(two_longs_dest[1] == 3);
    assert(vqueue_dequeue_some(queue, &num_dequeued, 2, two_more_longs_dest) == ENODATA);
    assert(num_dequeued == 1);
    assert(two_more_longs_dest[0] == 4);
    assert(two_more_longs_dest[1] == 67);

    free(queue);
}
}
