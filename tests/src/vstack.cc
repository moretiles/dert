//#include <vstack.h>

//#include <gtest/gtest.h>

namespace {
TEST(vstack, init) {
    Vstack *stack;
    stack = vstack_create(sizeof(long), 3);
    assert(stack != NULL);
    assert(vstack_len(stack) == 0);
    assert(vstack_cap(stack) == 3);

    long a = 1, b = 2, c = 3;
    assert(vstack_push(stack, &a) == 0);
    assert(vstack_push(stack, &b) == 0);
    assert(vstack_push(stack, &c) == 0);
    assert(vstack_len(stack) == 3);
    assert(vstack_cap(stack) == 3);

    long a_test, b_test, c_test;
    assert(vstack_top(stack, &c_test) == 0);
    assert(c_test == c);
    assert(vstack_pop(stack, &c_test) == 0);
    assert(vstack_pop(stack, &b_test) == 0);
    assert(vstack_pop(stack, &a_test) == 0);
    assert(a_test == a);
    assert(b_test == b);
    assert(c_test == c);

    assert(vstack_len(stack) == 0);
    assert(vstack_cap(stack) == 3);
    assert(vstack_destroy(stack) == 0);
}

TEST(vstack, initv) {
#define VSTACK_TEST_NUM_STACKS (20)
    Vstack *stacks;
    void *memory;
    long a, b, c, a_test, b_test, c_test;

    memory = calloc(1, vstack_advisev(VSTACK_TEST_NUM_STACKS, sizeof(long), 3));
    assert(memory != NULL);
    assert(vstack_initv(VSTACK_TEST_NUM_STACKS, &stacks, memory, sizeof(long), 3) == 0);

    for(size_t i = 0; i < VSTACK_TEST_NUM_STACKS; i++) {
        a = rand();
        assert(vstack_push(&(stacks[i]), &a) == 0);
        b = rand();
        assert(vstack_push(&(stacks[i]), &b) == 0);
        c = rand();
        assert(vstack_push(&(stacks[i]), &c) == 0);

        assert(vstack_pop(&(stacks[i]), &c_test) == 0);
        assert(c == c_test);
        assert(vstack_pop(&(stacks[i]), &b_test) == 0);
        assert(b == b_test);
        assert(vstack_pop(&(stacks[i]), &a_test) == 0);
        assert(a == a_test);
    }

    for(size_t i = 0; i < VSTACK_TEST_NUM_STACKS; i++) {
        vstack_deinit(&(stacks[i]));
    }
    free(stacks);
}
}
