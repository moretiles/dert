#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "tld.h"

char mt_tld_dumb[1 << 15] = { 0 };

__attribute__((const)) size_t mt_tld_dumb_hash(const char *str) {
    size_t result = 0;
    size_t i = 0;

    if(str == NULL) {
        return result;
    }

    while(str[i] != 0) {
        result <<= 1;
        result += str[i++];
        result %= (1 << 15);
    }

    return result;
}

void mt_tld_dumb_hash_underscore_string(char *str) {
    char local_buf[99] = { 0 };
    size_t num_bytes;
    assert(str != NULL);

    char *substr = strchr(str, '_');
    while(substr != NULL) {
        num_bytes = (substr - str);
        if(num_bytes > (99 - 1)) {
            num_bytes = 98;
        }
        memcpy(local_buf, str, num_bytes);
        local_buf[num_bytes] = 0;
        mt_tld_dumb[mt_tld_dumb_hash(local_buf)] = 1;
        if(*(substr + 1) == 0) {
            break;
        }

        str = substr + 1;
        substr = strchr(str, '_');
    }
    mt_tld_dumb[mt_tld_dumb_hash(str)] = 1;
}

__attribute__((pure)) bool mt_tld_dumb_hash_concat_check(int ignore_me, ...) {
    va_list vl;

    char buf[99] = { 0 };
    size_t pos = 0;
    size_t cap_remaining = 99 - 1;
    char *str = NULL;
    size_t str_len;

    va_start(vl, ignore_me);

    str = va_arg(vl, char*);
    while(str != NULL) {
        if(cap_remaining == 0) {
            break;
        }
        str_len = strlen(str);
        if(str_len > cap_remaining) {
            str_len = cap_remaining;
        }
        if(str_len == 0) {
            break;
        }
        memcpy(&(buf[pos]), str, str_len);
        pos += str_len;
        cap_remaining -= str_len;
        if(cap_remaining == 0) {
            break;
        }
        buf[pos++] = '-';
        cap_remaining--;

        str = va_arg(vl, char*);
    }
    if(pos > 0) {
        pos--;
    }
    buf[pos] = 0;
    return (bool) mt_tld_dumb[mt_tld_dumb_hash(buf)];
}

int mt_tld_trace_left_scope(const void *volatile *str) {
    if(*((char **) str) != NULL) {
        fprintf(stderr, "TRACE : LEAVE : %s\n", *((char **) str));
        fflush(stderr);
    }

    return 0;
}

#ifdef MT_TLD
__attribute__((constructor)) void mt_tld_constructor_set_dumb_hash_table(void) {
    mt_tld_dumb_hash_underscore_string(mt_tld_use_expanded_macro_as_string(MT_TLD));
}
#endif
