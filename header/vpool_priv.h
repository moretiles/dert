/*
 * vpool_priv.h -- Pool allocator for an arbitrary type
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Exists as an abstraction to hide some details from end-user
Vpool *_vpool_create(size_t num_items, size_t elem_size, Vpool_kind kind, Vpool *prev);

// Exists as an abstraction to hide some details from end-user
int _vpool_init(Vpool **dest, void *memory, size_t num_items, size_t elem_size, Vpool_kind kind, Vpool *prev);
