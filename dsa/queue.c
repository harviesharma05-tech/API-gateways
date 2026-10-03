#include "queue.h"
#include <stdlib.h>
void q_init(Queue *q) { q->head = q->tail = NULL; q->size = 0; }
int q_push(Queue *q, void *item) {
    QNode *n = (QNode *)malloc(sizeof(QNode));
    if (!n) return -1;
    n->item = item; n->next = NULL;
    if (q->tail) q->tail->next = n; else q->head = n;
    q->tail = n; q->size++;
    return 0;
}
void *q_pop(Queue *q) {
    if (!q->head) return NULL;
    QNode *n = q->head; void *item = n->item;
    q->head = n->next; if (!q->head) q->tail = NULL;
    free(n); q->size--;
    return item;
}
size_t q_size(const Queue *q) { return q->size; }
void q_free(Queue *q) { while (q->head) q_pop(q); }
