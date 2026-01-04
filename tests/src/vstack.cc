//#include <vstack.h>

//#include <gtest/gtest.h>

namespace {
TEST(vstack, init) {
    Vstack *stack;
    stack = vstack_create(sizeof(long), 3);
    ASSERT_NE(nullptr, stack);
    ASSERT_EQ(0, vstack_len(stack));
    ASSERT_EQ(3, vstack_cap(stack));

    long a = 1, b = 2, c = 3;
    ASSERT_EQ(0, vstack_push(stack, &a));
    ASSERT_EQ(0, vstack_push(stack, &b));
    ASSERT_EQ(0, vstack_push(stack, &c));
    ASSERT_EQ(3, vstack_len(stack));
    ASSERT_EQ(3, vstack_cap(stack));

    long a_test, b_test, c_test;
    ASSERT_EQ(0, vstack_top(stack, &c_test));
    ASSERT_EQ(c, c_test);
    ASSERT_EQ(0, vstack_pop(stack, &c_test));
    ASSERT_EQ(0, vstack_pop(stack, &b_test));
    ASSERT_EQ(0, vstack_pop(stack, &a_test));
    ASSERT_EQ(a, a_test);
    ASSERT_EQ(b, b_test);
    ASSERT_EQ(c, c_test);

    ASSERT_EQ(0, vstack_len(stack));
    ASSERT_EQ(3, vstack_cap(stack));
    ASSERT_EQ(0, vstack_destroy(stack));
}

TEST(vstack, initv) {
#define VSTACK_TEST_NUM_STACKS (20)
    Vstack *stacks;
    void *memory;
    long a, b, c, a_test, b_test, c_test;

    memory = calloc(1, vstack_advisev(VSTACK_TEST_NUM_STACKS, sizeof(long), 3));
    ASSERT_NE(nullptr, memory);
    ASSERT_EQ(0, vstack_initv(VSTACK_TEST_NUM_STACKS, &stacks, memory, sizeof(long), 3));

    for(size_t i = 0; i < VSTACK_TEST_NUM_STACKS; i++) {
        a = rand();
        ASSERT_EQ(0, vstack_push(&(stacks[i]), &a));
        b = rand();
        ASSERT_EQ(0, vstack_push(&(stacks[i]), &b));
        c = rand();
        ASSERT_EQ(0, vstack_push(&(stacks[i]), &c));

        ASSERT_EQ(0, vstack_pop(&(stacks[i]), &c_test));
        ASSERT_EQ(c_test, c);
        ASSERT_EQ(0, vstack_pop(&(stacks[i]), &b_test));
        ASSERT_EQ(b_test, b);
        ASSERT_EQ(0, vstack_pop(&(stacks[i]), &a_test));
        ASSERT_EQ(a_test, a);
    }

    for(size_t i = 0; i < VSTACK_TEST_NUM_STACKS; i++) {
        vstack_deinit(&(stacks[i]));
    }
    free(stacks);
}
}
