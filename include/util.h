#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdlib.h>

class util {
public:
    static int get_num_processors();
    static char* bin2hex(const unsigned char *p, size_t len);
    static bool hex2bin(unsigned char *p, const char *hexstr, size_t len);
    static uint32_t swab32(uint32_t v);
    static int timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y);
};

#endif
