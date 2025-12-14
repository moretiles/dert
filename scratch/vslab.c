#include <vslab.h>
#include <vslab_priv.h>

// The system page size is used to allocate one page worth of bytes at a time
/*
size_t vslab_system_page_size;

void __attribute__((constructor)) vslab_system_page_size_set_or_die(void);

void vslab_system_page_size_set_or_die(void) {
    int pagesize = getpagesize();
    assert(pagesize > 0);
    system_page_size = (size_t) pagesize;
}
*/
