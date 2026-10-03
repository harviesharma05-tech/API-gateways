#ifndef DYNARRAY_H
#define DYNARRAY_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Dynamic array of void* (grows by doubling). Used for the backend node list. */
typedef struct { void **data; size_t size, cap; } DynArray;
void   da_init(DynArray *a);
int    da_push(DynArray *a, void *item);        /* 0 = ok, -1 = out of memory */
void  *da_get(const DynArray *a, size_t i);     /* NULL if out of range */
size_t da_size(const DynArray *a);
void   da_free(DynArray *a);
#ifdef __cplusplus
}
#endif
#endif
