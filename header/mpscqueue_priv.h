/*
 * mpscqueue_priv.h -- Multi-producer, single-consumer queue
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

#pragma once

#include <stddef.h>

// Used to wrap calculations involving front and back
size_t mpscqueue_wrap(Mpscqueue *queue, size_t pos);
