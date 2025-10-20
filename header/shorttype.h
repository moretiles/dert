/*
 * shorttype.h -- Short, portable names for several types
 *
 * DERT - Miscellaneous Data Structures Library
 * https://github.com/moretiles/dert
 * Project licensed under Apache-2.0 license
 */

#include <stdint.h>
#include <stdbool.h>
#include <uchar.h>

// Shorten type names
#ifndef SHORT_NUMERIC_NAMES
    typedef int8_t i8;
    typedef int16_t i16;
    typedef int32_t i32;
    typedef int64_t i64;
    typedef __int128_t i128;
    
    typedef uint8_t u8;
    typedef uint16_t u16;
    typedef uint32_t u32;
    typedef uint64_t u64;
    typedef __uint128_t u128;
    
    #if defined(__clang__)
        // llvm documentation says float and double implement ieee-754-2008
        typedef _Float16 f16;
        typedef float f32;
        typedef double f64;
        typedef __float128 f128;
    #elif defined(_MSC_VER)
        // currently, I do no Windows development
        #define f16 #error "16 bit floats not directly supported by MSVC"; float
        typedef float f32;
        typedef double f64;
        #define f128 #error "128 bit floats not directly supported by MSVC"; float
    #else
	    // What you get with gcc
        typedef _Float16 f16;
        typedef _Float32 f32;
        typedef _Float64 f64;
        typedef _Float128 f128;
    #endif
#endif

#ifndef SHORT_CHAR_NAMES
    typedef char c8;
    typedef char16_t c16;
    typedef char32_t c32;
#endif
