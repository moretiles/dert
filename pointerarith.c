#include "pointerarith.h"

#include <stddef.h>

void *pointer_literal_addition(void *ptr, size_t increment){
    // ptr += x always increments ptr by (1 * x) when typeof(ptr) == char*
    char *alias = (char*) ptr;

    return alias + increment;
}
