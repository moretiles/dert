#include <stddef.h>

typedef struct vdll_node {
    void *data;
    struct vdll_node *prev;
    struct vdll_node *next;
} Vdll_node;


Vdll_node *vdll_node_init(size_t elem_size, int (*init)(void *arg));
int vdll_node_destroy(Vdll_node *node, size_t elem_size, int (*deinit)(void *arg));

int vdll_seek(Vdll *dll, size_t pos);
int vdll_wind(Vdll *dll, size_t increment);
int vdll_rewind(Vdll *dll, size_t increment);
