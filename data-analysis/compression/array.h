#ifndef ARRAY_H
#define ARRAY_H

#include <stddef.h>

typedef struct {
    void *ptr;
    size_t len;
    size_t capacity;
    size_t size;
} Array;

extern Array array_new(size_t capacity, size_t size);
extern void array_free(Array* arr);

extern void *get(Array arr, size_t idx);
extern void push(Array arr, void *item);

extern void swap(Array arr, size_t idx1, size_t idx2);

#endif