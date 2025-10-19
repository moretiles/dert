#include <stdlib.h>

#ifndef _CUSTOM_CSTRING_FUNCTIONS
#define _CUSTOM_CSTRING_FUNCTIONS 1
	char *cstrncpy(char *restrict dest, const char *restrict src, size_t m);
	void *cmemcpy(void *restrict dest, const void *restrict src, size_t count);
#endif
