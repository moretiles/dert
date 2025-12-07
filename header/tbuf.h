/*
 * tbuf.h -- Twin buffer. Useful for chaining together operations in which there are input/output buffers
 * This structure is not thread safe unless synchronization is done to make sure only one thread acts on it at a time
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

#pragma once

#include <stddef.h>

// Twin buffer
typedef struct tbuf {
    // A buffer
    char *A;

    // Current length of the A buffer
    // A_len < cap otherwise buffer overflow
    size_t A_len;

    // B buffer
    char *B;

    // Current length of the B buffer
    // B_len < cap otherwise buffer overflow
    size_t B_len;

    // Number of chars that can be stored in each buffer
    size_t cap;
} Tbuf;

// Allocates memory for and initializes a Tbuf.
Tbuf *tbuf_create(size_t cap);

// Advise how much memory a Tbuf in which each buffer has a capacity of cap chars needs
// Assumes that the same cap is used when calling tbuf_advise and tbuf_init
size_t tbuf_advise(size_t cap);

// Advise for many
size_t tbuf_advisev(size_t num_bufs, size_t cap);

// Initializes a Tbuf.
int tbuf_init(Tbuf **dest, void *memory, size_t cap);

// Initialize for many
int tbuf_initv(size_t num_bufs, Tbuf *dest[], void *memory, size_t cap);

// Deinitializes a Tbuf
void tbuf_deinit(Tbuf *twin);

/*
 * Destroys a Tbuf that was allocated by tbuf_create.
 * Please, only use with memory allocated by tbuf_create!
 */
void tbuf_destroy(Tbuf *twin);

// The A buffer of twin now points to what was B, the B buffer of twin now points to what was A
int tbuf_swap(Tbuf *twin);

// Get &(twin->T[twin->T_len])
char *tbuf_A(Tbuf *twin);
char *tbuf_B(Tbuf *twin);

// Get number of bytes unused
// Equivelent to twin->cap - twin->T_len
size_t tbuf_A_unused(Tbuf *twin);
size_t tbuf_B_unused(Tbuf *twin);

// Returns the total number of chars that can be stored in either A or B
size_t tbuf_cap(Tbuf *twin);
