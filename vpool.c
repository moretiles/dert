#include "vpool.h"
#include "vpool_priv.h"

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <pthread.h>

Vpool *vpool_create(size_t num_items, size_t elem_size, Vpool_functions *functions){
    return _vpool_create(num_items, elem_size, functions, NULL, NULL);
}

Vpool *_vpool_create(size_t num_items, size_t elem_size, Vpool_functions *functions, Vpool *prev, pthread_mutex_t *mutex){
    if(num_items == 0 || elem_size == 0){
        return NULL;
    }

    Vpool *vpool_created = calloc(1, sizeof(Vpool));
    if(vpool_created == NULL){
        return NULL;
    }

    if(elem_size < sizeof(void*)){
        elem_size = sizeof(void*);
    }
    void **const items = calloc(num_items, elem_size);
    if(items == NULL){
        free(vpool_created);
        return NULL;
    }

    if(mutex == NULL){
        mutex = calloc(1, sizeof(pthread_mutex_t));
        if(mutex == NULL){
            free(items);
            free(vpool_created);
            return NULL;
        }
        pthread_mutex_init(mutex, NULL);
    }

    /*
     * Use memcpy because struct vpool has const members. Strange but works.
     * If gcc/clang get better at identifying const access may need to make changes.
     */
    memcpy(vpool_created, &((Vpool){items, elem_size, 0, num_items, NULL, functions, prev, mutex}), sizeof(Vpool));

    return vpool_created;
}

void *vpool_alloc(Vpool **pool_ptr){
    return _vpool_alloc(pool_ptr, false);
}

void *_vpool_alloc(Vpool **pool_ptr, bool already_holding_mutex){
    if(pool_ptr == NULL){
        return NULL;
    }

    Vpool *pool = *pool_ptr;
    if(pool == NULL){
        return NULL;
    }

    // avoid locking twice when _vpool_alloc called recursively
    if(!already_holding_mutex){
        pthread_mutex_lock(pool->mutex);
    }

    void *allocated;
    if(pool->next_free != NULL){
        // pool->next_free is the top of a stack of allocated but not in-use elements
        allocated = pool->next_free;
        memcpy(&(pool->next_free), allocated, sizeof(void*));
        memset(allocated, 0, pool->element_size);
    } else if(pool->stored == pool->capacity){
        Vpool *new_pool = _vpool_create(pool->capacity * 2, pool->element_size, pool->functions, pool, pool->mutex);
        if(new_pool == NULL){
            return NULL;
        }

        *pool_ptr = new_pool;
        allocated = _vpool_alloc(pool_ptr, true);
    } else {
        allocated = ((void*) pool->items) + (pool->stored * pool->element_size);
        pool->stored++;
    }

    if(pool->functions != NULL && pool->functions->initialize_element != NULL){
        pool->functions->initialize_element(allocated);
    }

    // avoid unlocking early/twice when _vpool_alloc called recursively
    if(!already_holding_mutex){
        pthread_mutex_unlock(pool->mutex);
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

    pthread_mutex_lock(pool->mutex);

    if(pool->functions != NULL && pool->functions->deinitialize_element != NULL){
        pool->functions->deinitialize_element(elem);
    }
    memset(elem, 0, sizeof(void*));

    // pool->next_free is the top of a stack of allocated but not in-use elements
    if(pool->next_free != NULL){
        memcpy(elem, &(pool->next_free), sizeof(void*));
    }
    pool->next_free = elem;

    pthread_mutex_unlock(pool->mutex);

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

    pthread_mutex_lock(pool->mutex);

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

            if(pool_free_pointers_length > 1){
                qsort(pool_free_pointers, pool_free_pointers_length, pool->element_size, compare_pointers);
            }
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
                pools[pools_length++] = pool_iterate;
            }

            if(pools_length > 1){
                qsort(pools, pools_length, sizeof(Vpool*), vpool_compare_items_address);
            }
        }

        // Iterate over each pool's items deinitializing elements that have not already been deallocated.
        {
            size_t i = 0, j = 0;
            void **ptr;
            for(i = 0; i < pools_length; i++){
                for(ptr = pools[i]->items;
                        ptr < (pools[i]->items + pools[i]->stored);
                        ptr++){
                    if(ptr == pool_free_pointers[j]){
                        if((j + 1) < pool_free_pointers_length){
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

    // call free and NULL out pool_ptr
    {
        pthread_mutex_t *hold = pool->mutex;
        Vpool *pool_iterate, *prev = NULL;
        for(pool_iterate = pool; pool_iterate != NULL; pool_iterate = prev){
            prev = pool_iterate->prev;
            free(pool_iterate->items);
            free(pool_iterate);
        }

        *pool_ptr = NULL;
        pthread_mutex_unlock(hold);
        pthread_mutex_destroy(hold);
        free(hold);
    }

    return 0;
}


int compare_pointers(const void *a, const void *b) {
    if(a == b){
        return 0;
    } else if (a > b){
        return 1;
    } else {
        return -1;
    }
}

int vpool_compare_items_address(const void *a, const void *b){
    if(a == NULL || b == NULL){
        return 0;
    }

    return compare_pointers(((Vpool*) a)->items, ((Vpool*) b)->items);
}
