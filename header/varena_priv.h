/*
 * varena_priv.h -- Arena allocating memory for any type
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

struct varena_frame {
    size_t top;
    size_t bottom;
};

struct varena_frame *varena_frame_top(Varena *arena);
