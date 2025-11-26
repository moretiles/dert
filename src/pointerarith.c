#include <pointerarith.h>

#include <stddef.h>

void *pointer_literal_addition(void *ptr, size_t increment) {
    // ptr += x always increments ptr by (1 * x) when typeof(ptr) == char*
    char *alias = (char*) ptr;

    return alias + increment;
}

void *array_nth(void *array_base, size_t index, size_t item_size) {
    char *alias = (char*) array_base;

    return pointer_literal_addition(alias, index * item_size);
}
