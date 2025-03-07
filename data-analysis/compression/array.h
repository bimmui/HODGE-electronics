#ifndef ARRAY_H
#define ARRAY_H

#include <stddef.h>

typedef struct {
    void *ptr;
    size_t len;
    size_t capacity;
} Array;

extern Array array_new(size_t capacity);
extern void array_free(Array* arr);


#endif