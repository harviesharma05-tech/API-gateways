#ifndef HEAP_H
#define HEAP_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Binary min-heap keyed by double. Used to rank backend servers by score (lowest = best). */
typedef struct { double key; void *item; } HeapEntry;
typedef struct { HeapEntry *a; size_t size, cap; } MinHeap;
int    heap_init(MinHeap *h, size_t cap);
int    heap_push(MinHeap *h, double key, void *item);   /* O(log n) */
int    heap_pop(MinHeap *h, double *key, void **item);  /* O(log n); 0 = ok, -1 = empty */
size_t heap_size(const MinHeap *h);
void   heap_free(MinHeap *h);
#ifdef __cplusplus
}
#endif
#endif
