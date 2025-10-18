#include "varena.h"
#include "varena_priv.h"
#include "pointerarith.h"

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

Varena *varena_create(size_t num_bytes){
    Varena *ret;
    void *bytes;

    if(num_bytes == 0){
        return NULL;
    }

    ret = calloc(1, sizeof(Varena));
    if(ret == NULL){
        return NULL;
    }

    bytes = calloc(sizeof(char), num_bytes);
    if(bytes == NULL){
        free(ret);
        return NULL;
    }

    ret->bytes = bytes;
    ret->top = 0;
    ret->bottom = 0;
    ret->capacity = num_bytes;
    return ret;
}

void varena_destroy(Varena **arena_ptr){
    Varena *arena;
    if(arena_ptr == NULL){
        return;
    }

    arena = *arena_ptr;
    if(arena == NULL){
        return;
    }

    free(arena->bytes);
    memset(arena, 0, sizeof(Varena));
    free(arena);
    return;
}

size_t varena_arena_used(Varena *arena){
    if(arena == NULL){
        return 0;
    }

    return arena->bottom;
}

size_t varena_arena_unused(Varena *arena){
    if(arena == NULL){
        return 0;
    }

    return arena->capacity - arena->bottom;
}

size_t varena_arena_cap(Varena *arena){
    if(arena == NULL){
        return 0;
    }

    return arena->capacity;
}

size_t varena_frame_used(Varena *arena){
    struct varena_frame *frame;
    if(arena == NULL){
        return 0;
    }

    frame = varena_frame_top(arena);
    return arena->top - frame->top;
}

size_t varena_frame_unused(Varena *arena){
    if(arena == NULL){
        return 0;
    }

    return arena->bottom - arena->top;
}

struct varena_frame *varena_frame_top(Varena *arena){
    if(arena == NULL || arena->bottom < sizeof(struct varena_frame)){
        return NULL;
    }

    return pointer_literal_addition(arena->bytes, arena->bottom - sizeof(struct varena_frame));
}

int varena_claim(Varena **arena_ptr, size_t num_bytes){
    Varena *arena;
    struct varena_frame frame = { 0 };

    if(arena_ptr == NULL){
        return 1;
    }

    arena = *arena_ptr;
    if(arena == NULL){
        return 2;
    }

    if(num_bytes == 0){
        return 3;
    }

    // ensure space to store varena_frame in frame
    num_bytes += sizeof(struct varena_frame);
    if(arena->bottom + num_bytes > arena->capacity){
        return 4;
    }
    frame.top = arena->top;
    frame.bottom = arena->bottom;

    arena->top = arena->bottom;
    arena->bottom += num_bytes;
    memcpy(varena_frame_top(arena), &frame, sizeof(struct varena_frame));
    return 0;
}

int varena_disclaim(Varena **arena_ptr){
    Varena *arena;
    struct varena_frame *frame;

    if(arena_ptr == NULL){
        return 1;
    }

    arena = *arena_ptr;
    if(arena == NULL){
        return 2;
    }

    frame = varena_frame_top(arena);
    if(frame == NULL){
        return 3;
    }

    arena->top = frame->top;
    arena->bottom = frame->bottom;
    return 0;
}

void *varena_alloc(Varena **arena_ptr, size_t num_bytes){
    Varena *arena;
    void *ret;

    if(arena_ptr == NULL){
        return NULL;
    }

    arena = *arena_ptr;
    if(arena == NULL){
        return NULL;
    }

    if(arena->top + num_bytes > arena->bottom - sizeof(struct varena_frame)){
        return NULL;
    }
    ret = pointer_literal_addition(arena->bytes, arena->top);
    arena->top += num_bytes;

    return ret;
}
