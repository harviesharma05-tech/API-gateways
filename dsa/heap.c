#include "heap.h"
#include <stdlib.h>
int heap_init(MinHeap *h, size_t cap) {
    if (cap < 4) cap = 4;
    h->a = (HeapEntry *)malloc(cap * sizeof(HeapEntry));
    h->size = 0; h->cap = cap;
    return h->a ? 0 : -1;
}
int heap_push(MinHeap *h, double key, void *item) {
    if (h->size == h->cap) {
        HeapEntry *n = (HeapEntry *)realloc(h->a, h->cap * 2 * sizeof(HeapEntry));
        if (!n) return -1;
        h->a = n; h->cap *= 2;
    }
    size_t i = h->size++;
    while (i > 0) {                                   /* sift up */
        size_t p = (i - 1) / 2;
        if (h->a[p].key <= key) break;
        h->a[i] = h->a[p]; i = p;
    }
    h->a[i].key = key; h->a[i].item = item;
    return 0;
}
int heap_pop(MinHeap *h, double *key, void **item) {
    if (h->size == 0) return -1;
    if (key) *key = h->a[0].key;
    if (item) *item = h->a[0].item;
    HeapEntry last = h->a[--h->size];
    size_t i = 0;
    while (1) {                                       /* sift down */
        size_t l = 2 * i + 1, r = l + 1, m = i;
        double mk = last.key;
        if (l < h->size && h->a[l].key < mk) { m = l; mk = h->a[l].key; }
        if (r < h->size && h->a[r].key < mk) { m = r; }
        if (m == i) break;
        h->a[i] = h->a[m]; i = m;
    }
    if (h->size > 0) h->a[i] = last;
    return 0;
}
size_t heap_size(const MinHeap *h) { return h->size; }
void heap_free(MinHeap *h) { free(h->a); h->a = NULL; h->size = h->cap = 0; }
