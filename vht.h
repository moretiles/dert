#include <stddef.h>

typedef struct vht {
    void *keys;
    size_t key_size;
    void *vals;
    size_t val_size;
    size_t len;
    size_t cap;
    //also need functions
} Vht;

Vht *vht_create(size_t key_size, size_t val_size);
int vht_init(Vht *table, size_t key_size, size_t val_size);
void vht_deinit(Vht *table);
void vht_destroy(Vht *table);

int vht_get(Vht *table, void *key, void *dest);
void *vht_get_direct(Vht *table, void *key);
int vht_set(Vht *table, void *key, void *val);
int vht_del(Vht *table, void *key);

size_t vht_len(Vht *table);
