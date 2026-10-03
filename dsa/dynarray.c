#include "dynarray.h"
#include <stdlib.h>
void da_init(DynArray *a) { a->data = NULL; a->size = a->cap = 0; }
int da_push(DynArray *a, void *item) {
    if (a->size == a->cap) {
        size_t nc = a->cap ? a->cap * 2 : 4;
        void **nd = (void **)realloc(a->data, nc * sizeof(void *));
        if (!nd) return -1;
        a->data = nd; a->cap = nc;
    }
    a->data[a->size++] = item;
    return 0;
}
void *da_get(const DynArray *a, size_t i) { return i < a->size ? a->data[i] : NULL; }
size_t da_size(const DynArray *a) { return a->size; }
void da_free(DynArray *a) { free(a->data); a->data = NULL; a->size = a->cap = 0; }
