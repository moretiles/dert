#include <vpool.h>
#include <vpool_priv.h>
#include <pointerarith.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>

unsigned int fib(unsigned int n) {
    if (n <= 1) {
        return n;
    }

    unsigned int i = 1, j = 0, tmp;
    while (i < n) {
        tmp = i;
        i += j;
        j = tmp;

        i++;
    }

    return i;
}

Vpool *vpool_create(size_t num_items, size_t elem_size, Vpool_kind kind) {
    return _vpool_create(num_items, elem_size, kind, NULL);
}

Vpool *_vpool_create(size_t num_items, size_t elem_size, Vpool_kind kind, Vpool *prev) {
    if(num_items == 0 || elem_size == 0 || (kind == VPOOL_KIND_STATIC && prev != NULL)) {
        return NULL;
    }

    Vpool *vpool_created = calloc(1, vpool_advise(num_items, elem_size));
    if(vpool_created == NULL) {
        return NULL;
    }

    if(_vpool_init(&vpool_created, vpool_created, num_items, elem_size, kind, prev) != 0) {
        free(vpool_created);
        vpool_created = NULL;
    }

    return vpool_created;
}

size_t vpool_advise(size_t num_items, size_t elem_size) {
    if (elem_size < sizeof(void*)) {
        elem_size = sizeof(void*);
    }

    return 1 * (sizeof(Vpool) +
                (num_items * elem_size));
}

int vpool_init(Vpool **dest, void *memory, size_t num_items, size_t elem_size, Vpool_kind kind) {
    return _vpool_init(dest, memory, num_items, elem_size, kind, NULL);
}

int _vpool_init(Vpool **dest, void *memory, size_t num_items, size_t elem_size, Vpool_kind kind, Vpool *prev) {
    Vpool *pool;
    if(dest == NULL || *dest == NULL || memory == NULL || num_items == 0 || elem_size == 0 ||
            (kind == VPOOL_KIND_STATIC && prev != NULL)) {
        return EINVAL;
    }

    if (elem_size < sizeof(void*)) {
        elem_size = sizeof(void*);
    }

    pool = memory;
    pool->items = pointer_literal_addition(pool, sizeof(Vpool));
    pool->element_size = elem_size;
    pool->stored = 0;
    pool->capacity = num_items;
    pool->next_free = NULL;
    pool->kind = kind;
    pool->prev = prev;

    return 0;
}

int vpool_deinit(Vpool *pool) {
    Vpool *pool_iterate, *prev;

    if (pool == NULL) {
        return 0;
    }

    // DO NOT FREE THE FIRST VPOOL
    switch(pool->kind) {
    case VPOOL_KIND_DYNAMIC:
        for(
            pool_iterate = pool->prev;
            pool_iterate != NULL;
            pool_iterate = prev
        ) {
            prev = pool_iterate->prev;
            free(pool_iterate);
        }
        break;
    default:
        break;
    }

    return 0;
}

int vpool_destroy(Vpool *pool) {
    if (pool == NULL) {
        return 0;
    }

    // Free previous if VPOOL_KIND_DYNAMIC
    if(vpool_deinit(pool) != 0) {
        return EINVAL;
    }

    // Now free first pool
    free(pool);

    return 0;
}

void *vpool_alloc(Vpool *pool) {
    Vpool *new_pool, tmp;
    if(pool == NULL) {
        return NULL;
    }

    void *allocated;
    if(pool->next_free != NULL) {
        // pool->next_free is the top of a stack of allocated but not in-use elements
        allocated = pool->next_free;
        memcpy(&(pool->next_free), allocated, sizeof(void*));
        memset(allocated, 0, pool->element_size);
    } else if(vpool_full(pool)) {
        switch(pool->kind) {
        case VPOOL_KIND_DYNAMIC:
            new_pool = calloc(1, vpool_advise(2 * pool->capacity, pool->element_size));
            if(new_pool == NULL) {
                return NULL;
            }
            if(_vpool_init(&new_pool, new_pool, 2 * pool->capacity, pool->element_size, VPOOL_KIND_DYNAMIC, new_pool) != 0) {
                free(new_pool);
                new_pool = NULL;
                return NULL;
            }

            memcpy(&tmp, pool, sizeof(Vpool));
            memcpy(pool, new_pool, sizeof(Vpool));
            memcpy(new_pool, &tmp, sizeof(Vpool));

            allocated = vpool_alloc(pool);
            break;
        case VPOOL_KIND_STATIC:
        case VPOOL_KIND_GUIDED:
        default:
            // If invalid or VPOOL_KIND_STATIC then no more allocation can occur until some elements deallocate
            // The caller decides what to do if the pool is VPOOL_KIND_GUIDED

            return NULL;
            break;
        }
    } else {
        // pointer addition in this case scales by 1
        allocated = pointer_literal_addition(pool->items, pool->stored * pool->element_size);
        pool->stored++;
    }

    return allocated;
}

int vpool_dealloc(Vpool *pool, void *elem) {
    if(pool == NULL || elem == NULL) {
        return 2;
    }

    memset(elem, 0, sizeof(void*));

    // pool->next_free is the top of a stack of allocated but not in-use elements
    if(pool->next_free != NULL) {
        memcpy(elem, &(pool->next_free), sizeof(void*));
    }
    pool->next_free = elem;

    return 0;
}

bool vpool_full(Vpool *pool) {
    if(pool == NULL) {
        return false;
    }

    return pool->stored == pool->capacity;
}

int vpool_guided_extend(Vpool *pool, void *memory, size_t memory_size) {
    Vpool tmp;
    size_t num_items;
    if(pool == NULL || memory == NULL || pool->kind == VPOOL_KIND_STATIC) {
        return EINVAL;
    }

    num_items = (memory_size - sizeof(Vpool)) / pool->element_size;
    if(_vpool_init((Vpool**) &memory, memory, num_items, pool->element_size, VPOOL_KIND_GUIDED, memory) != 0) {
        return EINVAL;
    }
    memcpy(&tmp, pool, sizeof(Vpool));
    memcpy(pool, memory, sizeof(Vpool));
    memcpy(memory, &tmp, sizeof(Vpool));

    return 0;
}
