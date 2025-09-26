#include "shorttype.h"

#include <stddef.h>
#include <stdbool.h>

#define VHT_INITIAL_NUM_ELEMS (16)

#define FNV_PRIME_64 (1099511628091u)
#define FNV_OFFSET_BASIS_64 (14695981039346656037u)

struct vht_key_bf {
    bool occupied : 1;
};

u64 fnv(const char *data, size_t len);
int fnv_hash(Vht *table, const char *key, size_t len, u32 *offset, u32 *iterate, struct vht_key_bf **table_bf, void **table_key, void **table_val);
int fnv_next(Vht *table, u32 *offset, u32 *iterate, struct vht_key_bf **table_bf, void **table_key, void **table_val);
struct vht_key_bf *fnv_bf(Vht *table, u32 offset);
void *fnv_key(Vht *table, u32 offset);
void *fnv_val(Vht *table, u32 offset);

int vht_double(Vht **table_ptr);
Vht *_vht_init(size_t key_size, size_t val_size, size_t num_elems);

size_t vht_cap(Vht *table);
