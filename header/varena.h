/*
 * varena.h -- Arena allocating memory for any type
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

#include <stdint.h>
#include <stddef.h>

// All structs that are typedefined are public.

/*
 * Arena that allocates memory in variable length frames.
 */
typedef struct varena {
    // items stores what we allocate.
    void *bytes;

    // top of the current stack frame.
    size_t top;

    // bottom of the current stack frame;
    size_t bottom;

    // how many items we can possible store without reallocating.
    size_t capacity;
} Varena;

/*
 * Create new arena of num_bytes bytes.
 * Non-null pointer returned on success.
 */
Varena *varena_create(size_t num_bytes);

/*
 * Destroy existing arena.
 * Returns 0 and sets *arena_ptr = NULL on success.
 */
void varena_destroy(Varena **arena_ptr);

/*
 * Return the current number of bytes in-use across the whole arena
 */
size_t varena_arena_used(Varena *arena);

/*
 * Return the current number of bytes not-in-use across the whole arena
 */
size_t varena_arena_unused(Varena *arena);

/*
 * Return the current capacity of the whole arena
 */
size_t varena_arena_cap(Varena *arena);

/*
 * Return the current number of bytes in-use across the whole arena
 */
size_t varena_frame_used(Varena *arena);

/*
 * Return the current number of bytes not-in-use across the whole arena
 */
size_t varena_frame_unused(Varena *arena);

/*
 * Claim num_bytes bytes from arena and creates a new frame.
 * Returns 0 on success.
 */
int varena_claim(Varena **arena_ptr, size_t num_bytes);

/*
 * Remove claim on the top frame.
 * Returns 0 on success.
 */
int varena_disclaim(Varena **arena_ptr);

/*
 * Allocates num_bytes bytes from current arena frame.
 * Non-null pointer returned on success.
 */
void *varena_alloc(Varena **arena_ptr, size_t num_bytes);
