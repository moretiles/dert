/*
 * vqueue_priv.h -- Queue containing elements of an arbitrary type
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

#include <stddef.h>

// Used to wrap calculations involving front and back
size_t vqueue_wrap(Vqueue *queue, size_t pos);
