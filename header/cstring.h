/*
 * cstring.h -- Implement useful cstring functions
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

#pragma once

#include <stdlib.h>

	char *cstrncpy(char *restrict dest, const char *restrict src, size_t m);
	void *cmemcpy(void *restrict dest, const void *restrict src, size_t count);
