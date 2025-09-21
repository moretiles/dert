/*
 * assert.h -- Assert that is more friendly to debuggers
 *
 * Text EDitor
 * https://github.com/moretiles/ted
 * Project licensed under Apache-2.0 license
 */

#ifndef EXTERNAL_ASSERT
    #if defined(__clang__)
    	#define assert(statement) do { if (!(statement)) {  __builtin_debugtrap(); } } while(0)
    #elif defined(_MSC_VER)
    	#define assert(statement) do { if (!(statement)) {  __debugbreak(); } } while(0)
    #else
	// gcc has no good alternative, other than using signals
    	#include <assert.h>
    #endif
#endif
