#pragma once

// mine
#include <vpool.h>

// std
#include <stddef.h>

//vslab has many regions
//region has many subregions

// The system page size is used to allocate one page worth of bytes at a time
extern size_t vslab_system_page_size;

// Region of single sized slabs
typedef struct vslab_region {
    // Pool used to allocate from
    Vpool *pool;

    // Unique identifier associated with this region
    size_t identifier;

    // Cap for how much memory can be allocated
    // As all memory provided to region pools is in terms of system_page_size memory may be wasted
    //
    // For example, capping at (system_page_size + 1) bytes will cause two pages to be used.
    // These two pages created will then only allow at most one byte in the second page to be used wasting memory.
    size_t cap;
} Vslab_region;


// Slab allocator
typedef struct vslab {
    // backing storage for all slabs
    // provides system_page_size blocks of memory
    Vpool *pool;

    // regions
    struct tree_tree *regions;

    // total number of regions
    size_t num_regions;
} Vslab;

// Create a Vslab allocator
Vslab *vslab_create(size_t cap);

// Advise how much memory will be needed for a Vslab allocator
size_t vslab_advise(size_t cap);

// Initialize a Vslab allocator
int vslab_init(Vslab **dest, void *memory, size_t cap);

// Deinitialize a Vslab allocator
int vslab_deinit(Vslab *slab);

// Destroy a Vslab allocator created by vslab_create
int vslab_destroy(Vslab *slab);

// Allocate alloc_bytes (or more) from slab
// Will try to obtain memory from the smallest matching subregion
void *vslab_alloc(Vslab *slab, size_t alloc_bytes);

// Allocates from the region created with this region_identifier
// This is the best way to make allocations for fixed size types you want lots of
void *vslab_alloc_smart(Vslab *slab, size_t region_identifier);

// Free memory obtained using vslab_alloc
// Performs lookup to find what page ptr belongs to
// Does not call actually the libc free function
int vslab_free(Vslab *slab, void *ptr);

// Free memory obtained from calling vslab_alloc_smart with this region_identifier
int vslab_free_smart(Vslab *slab, size_t region_identifier, void *ptr);

// Get number of bytes in-use by regions of slab (always a multiple of system_page_size)
size_t vslab_len(Vslab *slab);

// Get byte capacity of slab (always a multiple of system_page_size)
size_t vslab_cap(Vslab *slab);

// Create region that allocates memory each alloc_size bytes in length
// If region_cap is not 0 then the amount of memory the region can grow to is limited
//
// Know when capping memory that allocations witin regions are in multiples of system_page_size bytes
// For example, capping at (system_page_size + 1) bytes will cause two pages to be used.
// These two pages created will then only allow at most one byte in the second page to be used wasting memory.
int vslab_region_create(Vslab *slab, size_t region_identifier, size_t alloc_size, size_t region_cap);

// Destroy region that allocates memory each alloc_size bytes in length
int vslab_region_destroy(Vslab *slab, size_t region_identifier);
