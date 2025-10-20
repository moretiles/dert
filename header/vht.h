/*
 * vht.h -- Hash table for an arbitrary key/value
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

#include <stddef.h>

typedef struct vht {
    // keys placed here.
    void *keys;

    // size in bytes of each key.
    size_t key_size;

    // vals placed here.
    void *vals;

    // size in bytes of each val.
    size_t val_size;

    // number of keys that have associated values.
    size_t len;

    /*
     * current capacity.
     * used to check if we need to resize up.
     */
    size_t cap;

    //also need functions
} Vht;

// Allocates memory for and initializes a Vht.
Vht *vht_create(size_t key_size, size_t val_size);

// Initializes a Vht.
int vht_init(Vht *table, size_t key_size, size_t val_size);

// Deinitializes a Vht.
void vht_deinit(Vht *table);

/*
 * Destroys a Vht that was allocated by vht_create.
 * Please, only use with memory allocated by vht_create!
 */
void vht_destroy(Vht *table);

// Copies the contents of the value associated with key in table to dest.
int vht_get(Vht *table, void *key, void *dest);

/*
 * Get the memory address of the value associated with key in table.
 * Should be used carefully as set and delete operations can freely overwrite contents at this memory location.
 * Thus, storing the pointer returned from this and then performing set and delete operations is a very bad idea!
 */
void *vht_get_direct(Vht *table, void *key);

// Copies src to the value associated with key in table.
int vht_set(Vht *table, void *key, void *src);

// Deletes the value associated with key from table.
int vht_del(Vht *table, void *key);

// Get number of keys that have associated values in table
size_t vht_len(Vht *table);
