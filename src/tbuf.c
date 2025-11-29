#include <tbuf.h>
#include <pointerarith.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

Tbuf *tbuf_create(size_t cap) {
    size_t memory_needed;
    Tbuf *twin = NULL;
    void *memory = NULL;
    int res = 0;
    bool complete = false;

    if(cap == 0) {
        return NULL;
    }

    memory_needed = tbuf_advise(cap);
    memory = calloc(1, memory_needed);
    if(memory == NULL) {
        return NULL;
    }

    res = tbuf_init(&twin, memory, cap);
    if(res != 0) {
        goto tbuf_create_error;
    }
    complete = true;

tbuf_create_error:
    if(!complete && twin != NULL) {
        free(memory);
        twin = NULL;
    }
    return twin;
}

size_t tbuf_advise(size_t cap) {
    return (1 * sizeof(Tbuf)) +
           (cap * sizeof(char)) +
           (cap * sizeof(char));
}

size_t tbuf_advisev(size_t num_bufs, size_t cap) {
    return num_bufs * ((1 * sizeof(Tbuf)) +
                       (cap * sizeof(char)) +
                       (cap * sizeof(char)));
}

int tbuf_init(Tbuf **dest, void *memory, size_t cap) {
    Tbuf *twin;
    char *A;
    char *B;
    void *next_unused_region;

    if(dest == NULL || memory ==  NULL || cap == 0) {
        return EINVAL;
    }

    next_unused_region = memory;
    next_unused_region = pointer_literal_addition(next_unused_region,
                         0);
    twin = (Tbuf *) next_unused_region;
    next_unused_region = pointer_literal_addition(next_unused_region,
                         (1 * sizeof(Tbuf)));
    A = (char *) next_unused_region;
    next_unused_region = pointer_literal_addition(next_unused_region,
                         (cap * sizeof(char)));
    B = (char *) next_unused_region;

    twin->A = A;
    twin->B = B;
    twin->cap = cap;

    *dest = twin;
    return 0;
}

int tbuf_initv(size_t num_bufs, Tbuf *dest[], void *memory, size_t cap) {
    Tbuf *twins;
    void *ptr;

    if(dest == NULL || memory ==  NULL || cap == 0) {
        return EINVAL;
    }

    twins = memory;
    ptr = pointer_literal_addition(memory, num_bufs * sizeof(Tbuf));
    for(size_t i = 0; i < num_bufs; i++) {
        twins[i].A = (char *) ptr;
        ptr = pointer_literal_addition(ptr, (cap * sizeof(char)));
        twins[i].B = (char *) ptr;
        ptr = pointer_literal_addition(ptr, (cap * sizeof(char)));
        twins[i].cap = cap;
    }

    *dest = twins;
    return 0;
}

void tbuf_deinit(Tbuf *twin) {
    if(twin == NULL) {
        return;
    }

    memset(twin->A, 0, twin->cap);
    memset(twin->B, 0, twin->cap);
    memset(twin, 0, sizeof(Tbuf));
    return;
}

void tbuf_destroy(Tbuf *twin) {
    if(twin == NULL) {
        return;
    }

    tbuf_deinit(twin);
    free(twin);
    return;
}

int tbuf_claim(Tbuf *twin, char **A, char **B) {
    if(twin == NULL || A == NULL || B == NULL) {
        return EINVAL;
    }

    *A = twin->A;
    *B = twin->B;
    return 0;
}

int tbuf_swap(Tbuf *twin) {
    char *tmp;

    if(twin == NULL) {
        return EINVAL;
    }

    tmp = twin->A;
    twin->A = twin->B;
    twin->B = tmp;

    return 0;
}

int tbuf_exchange(Tbuf *twin, char **A, char **B) {
    int res = 0;

    if(twin == NULL || A == NULL || B == NULL) {
        return EINVAL;
    }

    res = tbuf_claim(twin, A, B);
    if(res != 0) {
        goto tbuf_exchange_error;
    }

    res = tbuf_swap(twin);
    if(res != 0) {
        goto tbuf_exchange_error;
    }

tbuf_exchange_error:
    return res;
}

size_t tbuf_cap(Tbuf *twin) {
    return twin->cap;
}
