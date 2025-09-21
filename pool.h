#include <stdint.h>
#include <stddef.h>

// All structs that are typedefined are public.

/*
 * For operating on a given type, users can choose to implement these functions.
 */
typedef struct vpool_functions {
    // int initialize_element(void* element);
    int (*initialize_element)(void*);

    // int deinitialize_element(void* element);
    int (*deinitialize_element)(void*);
} Vpool_functions;

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

    // function pointers required to support various types with Vpools.
    struct vpool_functions *functions;

    // Previous pool
    struct vpool *prev;
} Vpool;

/*
 * Create new pool with num_items each of size elem_size, use provided functions.
 * Non-null pointer returned on success.
 */
Vpool *vpool_create(size_t num_items, size_t elem_size, Vpool_functions *functions);

/*
 * Obtain new allocation from pool.
 * Initialization function called before returning.
 * Non-null pointer returned on success.
 */
void *vpool_alloc(Vpool **pool_ptr);

/*
 * Deallocate existing element from pool.
 * Deinitialization function called on (*elem_ptr).
 * 0 on success.
 */
int vpool_dealloc(Vpool **pool_ptr, void *elem_ptr);

/*
 * Destroy existing pool.
 * Deinitialization function called on all not deallocated elements of pool.
 * Requires additional allocation of memory... if memory highly limited, consider exiting.
 * Returns 0 and sets *pool_ptr = NULL on success.
 */
int vpool_destroy(Vpool **pool_ptr);
