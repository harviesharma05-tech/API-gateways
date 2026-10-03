/* Unit tests for the C data structures.  Run:  make test */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dynarray.h"
#include "queue.h"
#include "hashmap.h"
#include "heap.h"
#include "tokenbucket.h"

int main(void) {
    /* dynamic array */
    DynArray a; da_init(&a);
    for (long i = 0; i < 100; i++) assert(da_push(&a, (void *)i) == 0);
    assert(da_size(&a) == 100 && (long)da_get(&a, 57) == 57 && da_get(&a, 100) == NULL);
    da_free(&a);

    /* queue: FIFO order */
    Queue q; q_init(&q);
    for (long i = 1; i <= 5; i++) q_push(&q, (void *)i);
    assert(q_size(&q) == 5);
    for (long i = 1; i <= 5; i++) assert((long)q_pop(&q) == i);
    assert(q_pop(&q) == NULL && q_size(&q) == 0);
    q_free(&q);

    /* hash map: put/get/replace/remove/resize */
    HashMap m; assert(hm_init(&m, 8) == 0);
    char key[32];
    for (long i = 0; i < 1000; i++) { sprintf(key, "10.0.0.%ld", i); assert(hm_put(&m, key, (void *)(i + 1)) == 0); }
    assert(hm_size(&m) == 1000 && m.nbuckets > 8);
    for (long i = 0; i < 1000; i++) { sprintf(key, "10.0.0.%ld", i); assert((long)hm_get(&m, key) == i + 1); }
    hm_put(&m, "10.0.0.5", (void *)999); assert((long)hm_get(&m, "10.0.0.5") == 999 && hm_size(&m) == 1000);
    assert((long)hm_remove(&m, "10.0.0.5") == 999 && hm_get(&m, "10.0.0.5") == NULL && hm_size(&m) == 999);
    assert(hm_get(&m, "nope") == NULL);
    hm_free(&m, NULL);

    /* min-heap: pops in ascending order */
    MinHeap h; heap_init(&h, 4);
    double vals[] = {5.5, 1.0, 9.0, 3.3, 7.7, 0.5, 2.2, 8.8};
    for (int i = 0; i < 8; i++) heap_push(&h, vals[i], NULL);
    double prev = -1, k;
    while (heap_pop(&h, &k, NULL) == 0) { assert(k >= prev); prev = k; }
    assert(heap_size(&h) == 0);
    heap_free(&h);

    /* token bucket: burst of 5, then refill 2 tokens/sec */
    TokenBucket tb; tb_init(&tb, 5, 2, 0.0);
    int ok = 0; for (int i = 0; i < 10; i++) ok += tb_allow(&tb, 0.0);
    assert(ok == 5);
    assert(tb_allow(&tb, 0.5) == 1 && tb_allow(&tb, 0.5) == 0);   /* 0.5 s -> 1 token */
    assert(tb_allow(&tb, 100.0) == 1 && tb.tokens <= 4.0 + 1e-9);  /* capped at capacity */

    puts("All C data-structure tests passed.");
    return 0;
}
