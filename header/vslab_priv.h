#pragma once

// mine
#include <vpool.h>

// std
#include <stddef.h>

// Set system_page_size or die
void vslab_system_page_size_set_or_die(void);

// Find the smallest region that fits alloc_bytes
Vslab_region *vslab_fit(Vslab *slab, size_t alloc_bytes);

// Find region ptr came from
Vslab_region *vslab_region_lookup(Vslab_region *region, void *ptr);

// Create region of slab
int _vslab_region_create(Vslab *slab, Vslab_region *region);

// Destroy region of slab
int _vslab_region_destroy(Vslab *slab, Vslab_region *region);

// Allocate once from this region
void *_vslab_region_alloc(Vslab *slab, Vslab_region *region);

// Free memory located in this region
int _vslab_region_free(Vslab *slab, Vslab_region *region);
