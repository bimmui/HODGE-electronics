#ifndef API_H
#define API_H

#include <stddef.h>

#include "array.h"

typedef void *(*create_func)(Array*, size_t);
typedef void *(*create_with_bits)(Array*, size_t, size_t);
typedef void *(*create_with_rms)(Array*, size_t, float);
typedef Array (*compress_func)(Array, size_t);

typedef struct {
    create_func create;
    create_with_bits create_with_bits;
    create_with_rms create_with_rms;
    compress_func compress;
} API;

#endif