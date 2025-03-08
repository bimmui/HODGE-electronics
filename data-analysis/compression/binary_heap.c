#include "stdbool.h"

#include "array.h"

typedef bool(*compare_func)(void *self, void *other);

typedef struct {
    Array arr;
    compare_func compare;
} BinaryHeap;

size_t left_child(size_t index) {
    return 2 * index + 1;
}

size_t right_child(size_t index) {
    return 2 * (index + 1);
}

size_t parent(size_t index) {
    return (index - 1) / 2;
}

void up_heap(BinaryHeap heap, size_t index) {
    while (index != 0) {
        size_t parent_idx = parent(index);
        if (heap.compare(get(heap.arr, index), get(heap.arr, parent_idx))) {
            swap(heap.arr, parent_idx, index);
        }
        index = parent_idx;
    }
}

void down_heap(BinaryHeap heap, size_t index) {
    while (index < heap.arr.len) {
        size_t left = left_child(index);
        if (left >= heap.arr.len) { return; }
        
        size_t best_idx = left;
        if (left + 1 < heap.arr.len && heap.compare(get(heap.arr, left), get(heap.arr, left + 1))) {
            best_idx += 1;
        }

        if (!heap.compare(get(heap.arr, index), get(heap.arr, best_idx))) {
            swap(heap.arr, index, best_idx);
        } else {
            return;
        }
    }
}

void insert(BinaryHeap heap, void *item) {
    push(heap.arr, item);
    up_heap(heap, heap.arr.len - 1);
}

void extract(BinaryHeap heap) {
    swap(heap.arr, 0, heap.arr.len - 1);
    heap.arr.len -= 1;
    down_heap(heap, 0);
}