/*
 * vpool.h -- Pool allocator for an arbitrary type
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <pthread.h>

// Controls how to handle when the pool is out of memory
typedef enum vpool_kind {
    // Total amount of memory allocated for items cannot change unless deinit/destroy called
    VPOOL_KIND_STATIC,

    // Total amount of memory can be increased using extend
    VPOOL_KIND_GUIDED,

    // Total amount of memory grows on demand
    VPOOL_KIND_DYNAMIC
} Vpool_kind;

/*
 * Pool that can allocate memory should it run out.
 * Uses void* for items but all items should be the same type.
 */
typedef struct vpool {
    // items stores what we allocate.
    void **items;

    // how large each element is. Must be greater-than-or-equal to sizeof(void*).
    size_t element_size;

    // how many elements currently stored.
    size_t stored;

    // how many items we can possible store without reallocating.
    size_t capacity;

    // next_free may may be NULL or point to anused region of memory in items.
    void *next_free;

    // Kind of Vpool
    Vpool_kind kind;

    // Only not NULL when VPOOL_KIND_DYNAMIC
    struct vpool *prev;
} Vpool;

// Create new pool with num_items each of size elem_size
// Non-null pointer returned on success.
Vpool *vpool_create(size_t num_items, size_t elem_size, Vpool_kind kind);

// Advise how much memory is needed
size_t vpool_advise(size_t num_items, size_t elem_size);

// Initialize new pool
int vpool_init(Vpool **dest, void *memory, size_t num_items, size_t elem_size, Vpool_kind kind);

// Deinitialize existing pool
int vpool_deinit(Vpool *pool_ptr);

// Destroy existing pool.
int vpool_destroy(Vpool *pool_ptr);

/*
 * Obtain new allocation from pool.
 * Initialization function called before returning.
 * Non-null pointer returned on success.
 */
void *vpool_alloc(Vpool *pool);

/*
 * Deallocate existing element from pool.
 * Deinitialization function called on (*elem_ptr).
 * 0 on success.
 */
int vpool_dealloc(Vpool *pool, void *elem_ptr);

// Returns true if pool is full
// Always returns false if pool is of kind VPOOL_KIND_DYNAMIC
bool vpool_full(Vpool *pool);

// Allows you to extend a Vpool of kind VPOOL_KIND_GUIDED when at capacity (vpool_full returns true)
// Memory is not kept track of and freed later when added using this method
int vpool_guided_extend(Vpool *pool, void *memory, size_t memory_size);
