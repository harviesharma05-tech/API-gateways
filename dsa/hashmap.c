#include "hashmap.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static uint64_t fnv1a(const char *s) {
    uint64_t h = 1469598103934665603ULL;
    while (*s) { h ^= (unsigned char)*s++; h *= 1099511628211ULL; }
    return h;
}
int hm_init(HashMap *m, size_t nb) {
    if (nb < 8) nb = 8;
    m->buckets = (HEntry **)calloc(nb, sizeof(HEntry *));
    m->nbuckets = nb; m->size = 0;
    return m->buckets ? 0 : -1;
}
static int resize(HashMap *m) {
    size_t nn = m->nbuckets * 2;
    HEntry **nb = (HEntry **)calloc(nn, sizeof(HEntry *));
    if (!nb) return -1;
    for (size_t i = 0; i < m->nbuckets; i++) {
        HEntry *e = m->buckets[i];
        while (e) { HEntry *nx = e->next; size_t j = fnv1a(e->key) % nn; e->next = nb[j]; nb[j] = e; e = nx; }
    }
    free(m->buckets); m->buckets = nb; m->nbuckets = nn;
    return 0;
}
int hm_put(HashMap *m, const char *key, void *val) {
    size_t i = fnv1a(key) % m->nbuckets;
    for (HEntry *e = m->buckets[i]; e; e = e->next)
        if (strcmp(e->key, key) == 0) { e->val = val; return 0; }
    if ((m->size + 1) * 4 > m->nbuckets * 3) { if (resize(m) != 0) return -1; i = fnv1a(key) % m->nbuckets; }
    HEntry *e = (HEntry *)malloc(sizeof(HEntry));
    if (!e) return -1;
    e->key = strdup(key);
    if (!e->key) { free(e); return -1; }
    e->val = val; e->next = m->buckets[i]; m->buckets[i] = e; m->size++;
    return 0;
}
void *hm_get(const HashMap *m, const char *key) {
    for (HEntry *e = m->buckets[fnv1a(key) % m->nbuckets]; e; e = e->next)
        if (strcmp(e->key, key) == 0) return e->val;
    return NULL;
}
void *hm_remove(HashMap *m, const char *key) {
    HEntry **pp = &m->buckets[fnv1a(key) % m->nbuckets];
    while (*pp) {
        HEntry *e = *pp;
        if (strcmp(e->key, key) == 0) { void *v = e->val; *pp = e->next; free(e->key); free(e); m->size--; return v; }
        pp = &e->next;
    }
    return NULL;
}
size_t hm_size(const HashMap *m) { return m->size; }
void hm_clear(HashMap *m, void (*free_val)(void *)) {
    for (size_t i = 0; i < m->nbuckets; i++) {
        HEntry *e = m->buckets[i];
        while (e) { HEntry *nx = e->next; if (free_val) free_val(e->val); free(e->key); free(e); e = nx; }
        m->buckets[i] = NULL;
    }
    m->size = 0;
}
void hm_free(HashMap *m, void (*free_val)(void *)) { hm_clear(m, free_val); free(m->buckets); m->buckets = NULL; m->nbuckets = 0; }
