#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Exists as an abstraction to hide mutex details from end-user
Vpool *_vpool_create(size_t num_items, size_t elem_size, Vpool_functions *functions, Vpool *prev, pthread_mutex_t *mutex);

// Exists as an abstraction to hide mutex details from end-user
void *_vpool_alloc(Vpool **pool_ptr, bool already_holding_mutex);

// Compare pointers by their magnitude
int compare_pointers(const void *a, const void *b);

// Compare the items pointer of two vpools by magnitude
int vpool_compare_items_address(const void *a, const void *b);
