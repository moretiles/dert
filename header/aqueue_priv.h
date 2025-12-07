/*
 * aqueue_priv.h -- Atomic queue designed for single producer, single consumer workflow
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

#pragma once

#include <stddef.h>

// Used to wrap calculations involving front and back
size_t aqueue_wrap(Aqueue *queue, size_t pos);
