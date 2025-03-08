#include "stdlib.h"

#include "array.h"

extern Array array_new(size_t capacity, size_t size) {
    void* ptr = calloc(capacity, size);
    return (Array) { .ptr = ptr, .len = 0, .capacity = capacity, .size = size };
}

extern void array_free(Array* arr) {
    free(arr->ptr);
    arr->ptr = NULL;
    arr->len = 0;
    arr->capacity = 0;
}

extern void *get(Array arr, size_t idx) {
    return (void*)((char *)arr.ptr + arr.size * idx);
}