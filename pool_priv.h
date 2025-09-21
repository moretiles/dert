#include <stdint.h>
#include <stddef.h>

// Compare pointers by their magnitude
int compare_pointers(const void *a, const void *b);

// Compare the items pointer of two vpools by magnitude
int vpool_compare_items_address(const void *a, const void *b);
