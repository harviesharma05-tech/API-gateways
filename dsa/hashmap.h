#ifndef HASHMAP_H
#define HASHMAP_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Hash table with separate chaining, string keys, FNV-1a hash, auto-resize at load 0.75.
   Used by the rate limiter: client IP -> token bucket. */
typedef struct HEntry { char *key; void *val; struct HEntry *next; } HEntry;
typedef struct { HEntry **buckets; size_t nbuckets, size; } HashMap;
int    hm_init(HashMap *m, size_t nbuckets);
int    hm_put(HashMap *m, const char *key, void *val);  /* replaces existing value */
void  *hm_get(const HashMap *m, const char *key);       /* NULL if missing */
void  *hm_remove(HashMap *m, const char *key);          /* returns removed value or NULL */
size_t hm_size(const HashMap *m);
void   hm_clear(HashMap *m, void (*free_val)(void *));  /* frees keys (+ values if fn given) */
void   hm_free(HashMap *m, void (*free_val)(void *));
#ifdef __cplusplus
}
#endif
#endif
