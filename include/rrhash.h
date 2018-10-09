#ifndef RRHASH_H
#define RRHASH_H

#include <stddef.h>

void rrhash_init();
void rrhash(const char *data, size_t length, unsigned char *hash);
void rrhash_release();

#endif
