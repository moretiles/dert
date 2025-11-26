#include <stddef.h>

/*
 * tbuf.h -- Twin buffer. Useful for chaining together operations in which there are input/output buffers
 * This structure is not thread safe unless synchronization is done to make sure only one thread acts on it at a time
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

// Twin buffer
typedef struct tbuf {
    // A buffer
    char *A;
    
    // B buffer
    char *B;

    // Number of chars that can be stored in each buffer
    size_t cap;
} Tbuf;

// Allocates memory for and initializes a Tbuf.
Tbuf *tbuf_create(size_t cap);

// Advise how much memory a Tbuf in which each buffer has a capacity of cap chars needs
// Assumes that the same cap is used when calling tbuf_advise and tbuf_init
size_t tbuf_advise(size_t cap);

// Initializes a Tbuf.
int tbuf_init(Tbuf **dest, void *memory, size_t cap);

// Deinitializes a Tbuf
void tbuf_deinit(Tbuf *twin);

/*
 * Destroys a Tbuf that was allocated by tbuf_create.
 * Please, only use with memory allocated by tbuf_create!
 */
void tbuf_destroy(Tbuf *twin);

// Derefernce and set A to the first buffer and B to the second buffer
int tbuf_claim(Tbuf *twin, char **A, char **B);

// The A buffer of twin now points to what was B, the B buffer of twin now points to what was A
int tbuf_swap(Tbuf *twin);

// Equivalent to calling tbuf_claim and immediately after that tbuf_swap
int tbuf_exchange(Tbuf *twin, char **A, char **B);

// Returns the total number of chars that can be stored in either A or B
size_t tbuf_cap(Tbuf *twin);
