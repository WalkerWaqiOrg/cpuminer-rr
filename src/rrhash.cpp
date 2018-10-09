#include <stdlib.h>
#include <dlfcn.h>
#include <stdio.h>

#include "rrhash.h"

#define RRHASH_LIB_PATH         "./librrhash.so"
#define RRHASH_LIB_PATH_WIN     "./librrhash.dll"
#define RRHASH_LIB_FUNC         "run_all"

typedef void (*RUN_RRHASH_FUNC)(const char *data, size_t length, unsigned char *hash);

void *rrhash_handle_;
RUN_RRHASH_FUNC rrhash_func;

void rrhash_init() {
#ifdef WIN32
    rrhash_handle_ = dlopen(RRHASH_LIB_PATH_WIN, RTLD_LAZY);
#else
    rrhash_handle_ = dlopen(RRHASH_LIB_PATH, RTLD_LAZY);
#endif
    if (!rrhash_handle_) {
        printf("DLL load error: %s\n", dlerror());
        exit(1);
    }

    rrhash_func = (RUN_RRHASH_FUNC)dlsym(rrhash_handle_, RRHASH_LIB_FUNC);
    if (dlerror() != NULL) {
        printf("DLL load error: %s\n", dlerror());
        exit(1);
    }
}

void rrhash(const char *data, size_t length, unsigned char *hash) {
    rrhash_func(data, length, hash);
}

void rrhash_release() {
    if (rrhash_handle_) {
        dlclose(rrhash_handle_);
    }
}
