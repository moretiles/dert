/*
 * pointerarith.h -- Perform void* pointer addition
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *pointer_literal_addition(void *ptr, size_t increment);

void *array_nth(void *array_base, size_t index, size_t item_size);

#ifdef __cplusplus
}
#endif
