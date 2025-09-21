#include "pool.h"
#include "pool_priv.h"

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

Vpool *vpool_create(size_t num_items, size_t elem_size, Vpool_functions *functions){
    if(num_items == 0 || elem_size == 0){
        return NULL;
    }

    Vpool *vpool_created = calloc(1, sizeof(Vpool));
    if(vpool_created == NULL){
        return NULL;
    }

    vpool_created->items = calloc(num_items, elem_size);
    if(vpool_created->items == NULL){
        free(vpool_created);
        return NULL;
    }

    if(elem_size < sizeof(void*)){
        elem_size = sizeof(void*);
    }
    vpool_created->element_size = elem_size;
    vpool_created->stored = 0;
    vpool_created->capacity = num_items;
    vpool_created->next_free = NULL;
    vpool_created->functions = functions;
    vpool_created->prev = NULL;
    return vpool_created;
}

void *vpool_alloc(Vpool **pool_ptr){
    if(pool_ptr == NULL){
        return NULL;
    }

    Vpool *pool = *pool_ptr;
    if(pool == NULL){
        return NULL;
    }

    void *allocated;
    if(pool->next_free != NULL){
        allocated = pool->next_free;
        memcpy(pool->next_free, allocated, sizeof(void*));
    } else if(pool->stored == pool->capacity){
        Vpool *new_pool = vpool_create(pool->capacity * 2, pool->element_size, pool->functions);
        if(new_pool == NULL){
            return NULL;
        }

        *pool_ptr = new_pool;
        allocated = vpool_alloc(pool_ptr);
    } else {
        allocated = ((void*) pool->items) + (pool->stored * pool->element_size);
        pool->stored++;
    }

    if(pool->functions != NULL && pool->functions->initialize_element != NULL){
        pool->functions->initialize_element(allocated);
    }
    return allocated;
}

int vpool_dealloc(Vpool **pool_ptr, void *elem){
    if(pool_ptr == NULL){
        return 1;
    }

    Vpool *pool = *pool_ptr;
    if(pool == NULL || elem == NULL){
        return 2;
    }

    if(pool->functions != NULL && pool->functions->deinitialize_element != NULL){
        pool->functions->deinitialize_element(elem);
    }
    memset(elem, 0, sizeof(void*));

    if(pool->next_free != NULL){
        memcpy(elem, &(pool->next_free), sizeof(void*));
    }
    pool->next_free = elem;
    return 0;
}

int vpool_destroy(Vpool **pool_ptr) {
    void **pool_free_pointers;
    Vpool **pools;
    size_t pool_free_pointers_length, pools_length;

    if (pool_ptr == NULL) {
        return 0;
    }

    Vpool *pool = *pool_ptr;
    if (pool == NULL) {
        return 0;
    }

    if(pool->functions != NULL && pool->functions->deinitialize_element != NULL){
        // Create array of all currently deallocated pointers from pool and children sorted by pointer magnitude.
        {
            pool_free_pointers_length = 0;
            size_t pool_free_pointers_capacity = 64;
            pool_free_pointers = calloc(pool_free_pointers_capacity, sizeof(void*));
            if(pool_free_pointers == NULL){
                return 1;
            }

            void **ptr = NULL;
            for(ptr = pool->next_free; ptr != NULL; ptr = *ptr){
                if(pool_free_pointers_length == pool_free_pointers_capacity){
                    pool_free_pointers_capacity *= 2;
                    pool_free_pointers = realloc(pool_free_pointers, pool_free_pointers_capacity * sizeof(void*));
                    if(pool_free_pointers == NULL){
                        return 3;
                    }
                    memset(pool_free_pointers + (pool_free_pointers_length * pool->element_size),
                            0,
                            (pool_free_pointers_capacity - pool_free_pointers_length) * pool->element_size);
                }
                pool_free_pointers[pool_free_pointers_length++] = ptr;
            }

            qsort(pool_free_pointers, pool_free_pointers_length, pool->element_size, compare_pointers);
        }

        // Create array of all current pools sorted by the magnitude of their starting pointer
        {
            pools_length = 0;
            size_t pools_capacity = 64;
            pools = calloc(pools_capacity, sizeof(void*));
            if(pools == NULL){
                free(pool_free_pointers);
                return 2;
            }

            Vpool *pool_iterate = NULL;
            for(pool_iterate = pool; pool_iterate != NULL; pool_iterate = pool_iterate->prev){
                if(pools_length == pools_capacity){
                    pools_capacity *= 2;
                    pools = realloc(pools, pools_capacity * sizeof(void*));
                    if(pools == NULL){
                        return 4;
                    }
                    memset(pools + (pools_length * pool->element_size),
                            0,
                            (pools_capacity - pools_length) * pool->element_size);
                }
                pools[pools_length++] = (void*) (pool_iterate->items);
            }

            qsort(pools, pools_length, sizeof(Vpool*), vpool_compare_items_address);
        }

        // Iterate over each pool's items deinitializing elements that have not already been deallocated.
        {
            size_t i = 0, j = 0;
            void **ptr;
            for(i = 0; i < pools_length; i++){
                for(ptr = pools[i]->items;
                        ptr < (pools[i]->items + (pools[i]->stored * pools[i]->element_size));
                        ptr += pools[i]->element_size){
                    if(ptr == pool_free_pointers[j]){
                        if(j < pool_free_pointers_length){
                            j++;
                        }
                    } else {
                        pool->functions->deinitialize_element(ptr);
                    }
                }
            }
            free(pool_free_pointers);
            free(pools);
        }
    }

    // just free pools->items and pools
    {
        Vpool *pool_iterate, *prev = NULL;
        for(pool_iterate = pool; pool_iterate != NULL; pool_iterate = prev){
            prev = pool_iterate->prev;
            free(pool_iterate->items);
            free(pool_iterate);
        }
    }

    *pool_ptr = NULL;

    return 0;
}

int compare_pointers(const void *a, const void *b) {
    if(a == b){
        return 0;
    } else if (a > b){
        return -1;
    } else {
        return 1;
    }
}

int vpool_compare_items_address(const void *a, const void *b){
    if(a == NULL && b == NULL){
        return 0;
    } else if (a == NULL){
        return 1;
    } else if (b == NULL){
        return -1;
    }

    return compare_pointers(((Vpool*) a)->items, ((Vpool*) b)->items);
}
