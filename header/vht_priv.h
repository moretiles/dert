/*
 * vht_priv.h -- Hash table for an arbitrary key/value
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

#include <shorttype.h>

#include <stddef.h>
#include <stdbool.h>

// Expected length as key for siphash
#define VHT_HASH_SALT_LEN_EXPECTED (16)
// Used as default size for Vht.
#define VHT_INITIAL_NUM_ELEMS (16)

/*
 * Global variable present in vht.c
 * Used as the "key" with siphash
 */
extern uint8_t vht_hash_salt[VHT_HASH_SALT_LEN_EXPECTED];

// Special secret bitfield placed before all keys stored in Vht
struct vht_key_bf {
    // Is the associated key set?
    bool occupied : 1;
};

void vht_hash_salt_set_or_die();

/* Internal FNV hashing calculation
 * Top 32 bits used to calculate iterate.
 * Bottom 32 bits used to calculate offset.
 */
u64 vht_hash_calc(const char *data, size_t len);

/*
 * Using the first len bytes of key calculate offset and iterate.
 * Then, find the vht_key_bf, key, and value at offset in table.
 */
int vht_hash_start(Vht *table, const char *key, size_t len, u32 *offset, u32 *iterate, struct vht_key_bf **table_bf, void **table_key, void **table_val);

/*
 * Apply an additional step of the FNV hashing calculation updating offset and iterate.
 * Get the vht_key_bf, key, and value at the offset in table.
 */
int vht_hash_next(Vht *table, u32 *offset, u32 *iterate, struct vht_key_bf **table_bf, void **table_key, void **table_val);

// Get vht_key_bf at offset in table.
struct vht_key_bf *vht_hash_bf(Vht *table, u32 offset);

// Get key at offset in table.
void *vht_hash_key(Vht *table, u32 offset);

// Get val at offset in table.
void *vht_hash_val(Vht *table, u32 offset);

// Grow size of Vht (by some amount).
int vht_double(Vht *table_ptr);

// Vht_create with parameterized starting number of elements.
Vht *_vht_create(size_t key_size, size_t val_size, size_t num_elems);

// Vht_init with parameterized starting number of elements.
int _vht_init(Vht *table, size_t key_size, size_t val_size, size_t num_elems);

// Current cap of table
size_t vht_cap(Vht *table);
