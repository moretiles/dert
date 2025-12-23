#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

extern char mt_tld_dumb[1 << 15];

#define mt_tld_expression_as_string(s) #s
#define mt_tld_use_expanded_macro_as_string(s) mt_tld_expression_as_string(s)

#ifdef MT_TLD
#define MT_LOG(...) \
do { \
  if(mt_tld_dumb_hash_concat_check(0, "log", __func__, NULL)) { \
    fprintf(stderr, "LOG : %s : ", __func__); \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n"); \
    fflush(stderr); \
  } \
} while(0)

#define MT_TRACE() \
const void *volatile ___we_do_tracing __attribute__((__cleanup__(mt_tld_trace_left_scope))) = NULL; \
do { \
  if(mt_tld_dumb_hash_concat_check(0, "trace", __func__, NULL)) { \
    fprintf(stderr, "TRACE : ENTER : %s\n", __func__); \
    ___we_do_tracing = __func__; \
    fflush(stderr); \
  } \
} while(0)

#define MT_DEBUG(tag) if(mt_tld_dumb_hash_concat_check(0, "debug", __func__, tag, NULL))

__attribute__((constructor)) void mt_tld_constructor_set_dumb_hash_table(void);
#else
#define MT_LOG(...) do {} while(0)
#define MT_TRACE() do {} while(0)
#define MT_DEBUG(tag) if(0)
#endif

__attribute__((const)) size_t mt_tld_dumb_hash(const char *str);
void mt_tld_dumb_hash_underscore_string(char *str);
__attribute__((pure)) bool mt_tld_dumb_hash_concat_check(int ignore_me, ...);
int mt_tld_trace_left_scope(const void *volatile *str);
