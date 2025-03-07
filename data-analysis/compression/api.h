#ifndef API_H
#define API_H

#include <stddef.h>

#include "array.h"

typedef void *(*create_func)(Array*, size_t);
typedef Array (*compress_func)(Array, size_t);

typedef struct {
    create_func create;
    compress_func compress;
} API;

#endif