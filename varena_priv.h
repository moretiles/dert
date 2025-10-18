#include <stddef.h>
#include <stdint.h>

struct varena_frame {
    size_t top;
    size_t bottom;
};

struct varena_frame *varena_frame_top(Varena *arena);
