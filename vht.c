#include "vht.h"
#include "vht_priv.h"
#include "shorttype.h"
#include "pointerarith.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/*
typedef struct vht {
    Varray *keys;
    Varray *vals;
    //also need functions
} Vht;
*/

u64 fnv(const char *data, size_t len){
    u128 hash = FNV_OFFSET_BASIS_64;
    u64 tmp = 0;
    size_t i = 0;

    if (data == NULL || len == 0) {
        return hash;
    }

    // use eight byte chunks
	for(i = 0; i < (len / 8); i++){
        hash *= FNV_PRIME_64;
        hash ^= data[i * 8];
	}

    // handle remaining data
    if(len != 0){
        memcpy(&tmp, &(data[i * 8]), len % 8);
        hash *= FNV_PRIME_64;
        hash ^= tmp;
    }

    return hash;
}

int fnv_hash(Vht *table, const char *key, size_t len, u32 *offset, u32 *iterate, struct vht_key_bf **table_bf, void **table_key, void **table_val){
    if(table == NULL || key == NULL || offset == NULL || iterate == NULL || table_bf == NULL || table_key == NULL || table_val == NULL){
        return 1;
    }

    // offset gets low bits, iterate gets high bits
    u64 hash = fnv(key, len);
    *offset = hash >> 0;
    *iterate = hash >> 32;

    // iterate must be odd otherwise may not reach all indices
    *iterate |= 0x1;

    *table_bf = fnv_bf(table, *offset);
    *table_key = fnv_key(table, *offset);
    *table_val = fnv_val(table, *offset);
    if(table_bf == NULL || table_key == NULL || table_val == NULL){
        return 2;
    }
    return 0;
}

int fnv_next(Vht *table, u32 *offset, u32 *iterate, struct vht_key_bf **table_bf, void **table_key, void **table_val){
    if(table == NULL || offset == NULL || iterate == NULL || table_bf == NULL || table_key == NULL || table_val == NULL){
        return 1;
    }

    *offset += *iterate;

    *table_bf = fnv_bf(table, *offset);
    *table_key = fnv_key(table, *offset);
    *table_val = fnv_val(table, *offset);
    if(table_bf == NULL || table_key == NULL || table_val == NULL){
        return 2;
    }

    return 0;
}

struct vht_key_bf *fnv_bf(Vht *table, u32 offset){
    struct vht_key_bf *bf;
    if(table == NULL){
        return NULL;
    }

    bf = pointer_literal_addition(table->keys, (offset % table->cap) * (sizeof(struct vht_key_bf) + table->key_size));
    return bf;
}

void *fnv_key(Vht *table, u32 offset){
    void *key;
    if(table == NULL){
        return NULL;
    }

    key = fnv_bf(table, offset);
    if(key == NULL){
        return NULL;
    }
    key = pointer_literal_addition(key, sizeof(struct vht_key_bf));
    return key;
}

void *fnv_val(Vht *table, u32 offset){
    void *val;
    if(table == NULL){
        return NULL;
    }

    val = pointer_literal_addition(table->vals, (offset % table->cap) * table->val_size);
    return val;
}

Vht *vht_create(size_t key_size, size_t val_size){
    return _vht_create(key_size, val_size, VHT_INITIAL_NUM_ELEMS);
}

Vht *_vht_create(size_t key_size, size_t val_size, size_t num_elems){
    Vht *ret = calloc(1, sizeof(Vht));
    if(ret == NULL){
        return NULL;
    }

    if(_vht_init(ret, key_size, val_size, num_elems) != 0){
        free(ret);
        return NULL;
    }
    return ret;
}

int vht_init(Vht *table, size_t key_size, size_t val_size){
    return _vht_init(table, key_size, val_size, VHT_INITIAL_NUM_ELEMS);
}

int _vht_init(Vht *table, size_t key_size, size_t val_size, size_t num_elems){
    if(key_size == 0 || val_size == 0){
        return 1;
    }

    if(num_elems == 0){
        num_elems = VHT_INITIAL_NUM_ELEMS;
    }

    // Need bitfield to store additional information
    table->key_size = key_size;
    table->keys = calloc(num_elems, sizeof(struct vht_key_bf) + key_size);
    if(table->keys == NULL){
        free(table);
        return 2;
    }

    table->val_size = val_size;
    table->vals = calloc(num_elems, val_size);
    if(table->vals == NULL){
        free(table->keys);
        free(table);
        return 3;
    }

    table->len = 0;
    table->cap = num_elems;
    return 0;
}

void vht_deinit(Vht *table){
    if(table == NULL){
        return;
    }

    free(table->keys);
    free(table->vals);
    return;
}

void vht_destroy(Vht *table){
    if(table == NULL){
        return;
    }

    vht_deinit(table);
    free(table);
    return;
}

int vht_get(Vht *table, void *key, void *dest){
    void *src;
    if(table == NULL || key == NULL || dest == NULL){
        return 1;
    }

    src = vht_get_direct(table, key);
    if(src == NULL){
        return 2;
    }

    if(memcpy(dest, src, table->val_size) != dest){
        return 3;
    }
    return 0;
}

void *vht_get_direct(Vht *table, void *key){
    size_t remaining_guesses;
    u32 offset, iterate;
    struct vht_key_bf *table_bf;
    void *table_key, *table_val;
    if(table == NULL || key == NULL){
        return NULL;
    }

    remaining_guesses = table->cap;
    if(fnv_hash(table, key, table->key_size, &offset, &iterate, &table_bf, &table_key, &table_val) != 0){
        return NULL;
    }

    while(remaining_guesses > 0 && table_bf != NULL && table_bf->occupied && table_key != NULL && table_val != NULL){
        if(!memcmp(key, table_key, table->key_size)){
            return table_val;
        }
        remaining_guesses--;
        if(fnv_next(table, &offset, &iterate, &table_bf, &table_key, &table_val) != 0){
            return NULL;
        }
    }

    return NULL;
}

int vht_set(Vht *table, void *key, void *src){
    size_t remaining_guesses;
    u32 offset, iterate;
    struct vht_key_bf *table_bf;
    void *table_key, *table_val;
    if(table == NULL || key == NULL || src == NULL){
        return 1;
    }

    if((table->len * 4) >= table->cap){
        vht_double(table);
    }

    remaining_guesses = table->cap;
    if(fnv_hash(table, key, table->key_size, &offset, &iterate, &table_bf, &table_key, &table_val) != 0){
        return 2;
    }

    while(remaining_guesses > 0 && table_bf != NULL && table_key != NULL && table_val != NULL){
        if(!table_bf->occupied){
            memset(table_bf, 0, sizeof(struct vht_key_bf));
            table_bf->occupied = true;
            memcpy(table_key, key, table->key_size);
            memcpy(table_val, src, table->val_size);
            table->len++;
            return 0;
        } else if(table_bf->occupied && !memcmp(key, table_key, table->key_size)){
            memcpy(table_val, src, table->val_size);
            return 0;
        }

        remaining_guesses--;
        if(fnv_next(table, &offset, &iterate, &table_bf, &table_key, &table_val) != 0){
            return 4;
        }
    }

    return 5;
}

int vht_del(Vht *table, void *key){
    size_t remaining_guesses;
    u32 offset, iterate;
    struct vht_key_bf *table_bf;
    void *table_key, *table_val;
    if(table == NULL || key == NULL){
        return 1;
    }

    remaining_guesses = table->cap;
    if(fnv_hash(table, key, table->key_size, &offset, &iterate, &table_bf, &table_key, &table_val) != 0){
        return 2;
    }

    while(remaining_guesses > 0 && table_bf != NULL && table_bf->occupied && table_key != NULL){
        if(!memcmp(key, table_key, table->key_size)){
            memset(table_bf, 0, sizeof(struct vht_key_bf));
            memset(table_key, 0, table->key_size);
            memset(table_val, 0, table->val_size);
            table->len--;
            return 0;
        }
        remaining_guesses--;
        if(fnv_next(table, &offset, &iterate, &table_bf, &table_key, &table_val) != 0){
            return 3;
        }
    }

    return 4;
}

int vht_double(Vht *table){
    Vht new_table;
    u32 offset, iterate;
    struct vht_key_bf *table_bf;
    void *random_sequence, *table_key, *table_val;
    size_t remaining_positions;
    if(table == NULL){
        return 1;
    }

    random_sequence = malloc(table->key_size);
    if(_vht_init(&new_table, table->key_size, table->val_size, 4 * table->cap) != 0){
        return 2;
    }
    if(random_sequence == NULL){
        free(random_sequence);
        return 3;
    }

    remaining_positions = vht_cap(table);
    if(fnv_hash(table, random_sequence, table->key_size, &offset, &iterate, &table_bf, &table_key, &table_val) != 0){
        return 4;
    }
    free(random_sequence);
    while(remaining_positions-- > 0){
        if(fnv_next(table, &offset, &iterate, &table_bf, &table_key, &table_val) != 0){
            return 4;
        }

        if(table_bf->occupied){
            if(vht_set(&new_table, table_key, table_val) != 0){
                    return 5;
            }
        } else {
            continue;
        }
    }

    vht_deinit(table);
    if(memcpy(table, &new_table, sizeof(Vht)) != table){
        return 7;
    }
    return 0;
}

size_t vht_len(Vht *table){
    if(table == NULL){
        return 0;
    }

    return table->len;
}

size_t vht_cap(Vht *table){
    if(table == NULL){
        return 0;
    }

    return table->cap;
}
