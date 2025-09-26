#include <stddef.h>

typedef struct vht {
    //Varray *keys;
    //Varray *vals;
    void *keys;
    size_t key_size;
    void *vals;
    size_t val_size;
    size_t len;
    size_t cap;
    //also need functions
} Vht;

Vht *vht_init(size_t key_size, size_t val_size);
int vht_destroy(Vht *table);

void *vht_get(Vht *table, void *key);
int vht_set(Vht **table_ptr, void *key, void *val);
int vht_del(Vht *table, void *key);

size_t vht_len(Vht *table);
