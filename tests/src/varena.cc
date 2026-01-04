//#include <varena.h>
//#include <pointerarith.h>

//#include <gtest/gtest.h>

namespace {
TEST(varena, test) {
#define FIRST_FRAME_SIZE (0x04)
#define SECOND_FRAME_SIZE (0x04)
#define THIRD_FRAME_SIZE (0x10)
#define A_CONSTANT (0x01234567)
#define B_CONSTANT (0x89ABCDEF)
#define C_CONSTANT (0x00112233)
#define D_CONSTANT (0x44556677)
#define E_CONSTANT (0x8899AABB)
#define F_CONSTANT (0xCCDDEEFF)

    int32_t *a, *b, *c, *d, *e, *f;
    Varena *arena = varena_create(999);
    ASSERT_NE(nullptr, arena);
    ASSERT_EQ(0, varena_arena_used(arena));
    ASSERT_EQ(999, varena_arena_unused(arena));
    ASSERT_EQ(999, varena_arena_cap(arena));

    ASSERT_EQ(0, varena_claim(&arena, FIRST_FRAME_SIZE));
    ASSERT_EQ(0, varena_frame_used(arena));
    ASSERT_GE(varena_frame_unused(arena), FIRST_FRAME_SIZE);
    a = (int32_t *) varena_alloc(&arena, sizeof(int32_t));
    ASSERT_NE(nullptr, a);
    ASSERT_GE(varena_frame_used(arena), FIRST_FRAME_SIZE);
    ASSERT_EQ(0, varena_frame_unused(arena));
    ASSERT_LT(a, pointer_literal_addition(arena->bytes, arena->bottom));
    *a = A_CONSTANT;

    ASSERT_EQ(0, varena_claim(&arena, SECOND_FRAME_SIZE));
    ASSERT_EQ(0, varena_frame_used(arena));
    ASSERT_GE(varena_frame_unused(arena), SECOND_FRAME_SIZE);
    b = (int32_t *) varena_alloc(&arena, sizeof(int32_t));
    ASSERT_NE(nullptr, b);
    ASSERT_GE(varena_frame_used(arena), SECOND_FRAME_SIZE);
    ASSERT_EQ(0, varena_frame_unused(arena));
    ASSERT_LT(b, pointer_literal_addition(arena->bytes, arena->bottom));
    *b = B_CONSTANT;

    ASSERT_EQ(0, varena_claim(&arena, THIRD_FRAME_SIZE));
    ASSERT_EQ(0, varena_frame_used(arena));
    ASSERT_GE(varena_frame_unused(arena), THIRD_FRAME_SIZE);
    c = (int32_t *) varena_alloc(&arena, sizeof(int32_t));
    *c = C_CONSTANT;
    d = (int32_t *) varena_alloc(&arena, sizeof(int32_t));
    *d = D_CONSTANT;
    e = (int32_t *) varena_alloc(&arena, sizeof(int32_t));
    *e = E_CONSTANT;
    f = (int32_t *) varena_alloc(&arena, sizeof(int32_t));
    *f = F_CONSTANT;
    ASSERT_GE(varena_frame_used(arena), THIRD_FRAME_SIZE);
    ASSERT_EQ(0, varena_frame_unused(arena));

    ASSERT_NE(nullptr, c);
    ASSERT_EQ(*c, C_CONSTANT);
    ASSERT_NE(nullptr, d);
    ASSERT_EQ(*d, D_CONSTANT);
    ASSERT_NE(nullptr, e);
    ASSERT_EQ(*e, E_CONSTANT);
    ASSERT_NE(nullptr, f);
    ASSERT_EQ(*f, F_CONSTANT);
    ASSERT_LT(c, pointer_literal_addition(arena->bytes, arena->bottom));
    ASSERT_LT(f, pointer_literal_addition(arena->bytes, arena->bottom));
    ASSERT_GE(varena_arena_used(arena), FIRST_FRAME_SIZE + SECOND_FRAME_SIZE + THIRD_FRAME_SIZE);
    ASSERT_NE(0, varena_arena_unused(arena));
    ASSERT_EQ(999, varena_arena_cap(arena));
    ASSERT_EQ(0, varena_disclaim(&arena));

    ASSERT_EQ(*b, B_CONSTANT);
    ASSERT_EQ(0, varena_disclaim(&arena));

    ASSERT_EQ(*a, A_CONSTANT);
    ASSERT_EQ(0, varena_disclaim(&arena));

    varena_destroy(&arena);
    return;
}
}
