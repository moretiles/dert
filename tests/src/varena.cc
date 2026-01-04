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
    assert(arena != NULL);
    assert(varena_arena_used(arena) == 0);
    assert(varena_arena_unused(arena) == 999);
    assert(varena_arena_cap(arena) == 999);

    assert(varena_claim(&arena, FIRST_FRAME_SIZE) == 0);
    assert(varena_frame_used(arena) == 0);
    assert(varena_frame_unused(arena) >= FIRST_FRAME_SIZE);
    a = (int32_t *) varena_alloc(&arena, sizeof(int32_t));
    assert(a != NULL);
    assert(varena_frame_used(arena) >= FIRST_FRAME_SIZE);
    assert(varena_frame_unused(arena) == 0);
    assert((void*) a < pointer_literal_addition(arena->bytes, arena->bottom));
    *a = A_CONSTANT;

    assert(varena_claim(&arena, SECOND_FRAME_SIZE) == 0);
    assert(varena_frame_used(arena) == 0);
    assert(varena_frame_unused(arena) >= SECOND_FRAME_SIZE);
    b = (int32_t *) varena_alloc(&arena, sizeof(int32_t));
    assert(b != NULL);
    assert(varena_frame_used(arena) >= SECOND_FRAME_SIZE);
    assert(varena_frame_unused(arena) == 0);
    assert((void*) b < pointer_literal_addition(arena->bytes, arena->bottom));
    *b = B_CONSTANT;

    assert(varena_claim(&arena, THIRD_FRAME_SIZE) == 0);
    assert(varena_frame_used(arena) == 0);
    assert(varena_frame_unused(arena) >= THIRD_FRAME_SIZE);
    c = (int32_t *) varena_alloc(&arena, sizeof(int32_t));
    *c = C_CONSTANT;
    d = (int32_t *) varena_alloc(&arena, sizeof(int32_t));
    *d = D_CONSTANT;
    e = (int32_t *) varena_alloc(&arena, sizeof(int32_t));
    *e = E_CONSTANT;
    f = (int32_t *) varena_alloc(&arena, sizeof(int32_t));
    *f = F_CONSTANT;
    assert(varena_frame_used(arena) >= THIRD_FRAME_SIZE);
    assert(varena_frame_unused(arena) == 0);

    assert(c != NULL);
    assert((*c = C_CONSTANT));
    assert(d != NULL);
    assert((*d = D_CONSTANT));
    assert(e != NULL);
    assert((*e = E_CONSTANT));
    assert(f != NULL);
    assert((*f = F_CONSTANT));
    assert((void*) c < pointer_literal_addition(arena->bytes, arena->bottom));
    assert((void*) f < pointer_literal_addition(arena->bytes, arena->bottom));
    assert(varena_arena_used(arena) >= FIRST_FRAME_SIZE + SECOND_FRAME_SIZE + THIRD_FRAME_SIZE);
    assert(varena_arena_unused(arena) != 0);
    assert(varena_arena_cap(arena) == 999);
    assert(varena_disclaim(&arena) == 0);

    assert((*b = B_CONSTANT));
    assert(varena_disclaim(&arena) == 0);

    assert((*a = A_CONSTANT));
    assert(varena_disclaim(&arena) == 0);

    varena_destroy(&arena);
    return;
}
}
