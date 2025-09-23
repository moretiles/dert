/*
 * dll.h -- Vdll and Vdll_functions
 *
 * c_misc - Some useful C files I wrote
 * https://github.com/moretiles/c_misc
 * Project licensed under Apache-2.0 license
 */

#include <stddef.h>

typedef struct vdll_functions {
	int (*init)(void *arg);
	int (*deinit)(void *arg);
} Vdll_functions;

typedef struct vdll {
	struct vdll_node *ptr;
	size_t elem_size;
	size_t pos;
	size_t cap;
	struct vdll_functions *functions;
} Vdll;

Vdll *vdll_init(size_t elem_size, Vdll_functions *functions);
int vdll_get(Vdll *dll, size_t pos, void *dest);
int vdll_set(Vdll *dll, size_t pos, void *src);
int vdll_destroy(Vdll *dll);

size_t vdll_len(Vdll *dll);
int vdll_insert(Vdll *dll, size_t pos, size_t num_elems);
int vdll_delete(Vdll *dll, size_t pos, size_t num_elems);
int vdll_grow(Vdll *dll, size_t num_elems);
int vdll_shrink(Vdll *dll, size_t num_elems);
