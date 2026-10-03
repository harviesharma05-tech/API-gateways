#ifndef QUEUE_H
#define QUEUE_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* FIFO queue built on a singly linked list. Used as the thread-pool task queue. */
typedef struct QNode { void *item; struct QNode *next; } QNode;
typedef struct { QNode *head, *tail; size_t size; } Queue;
void   q_init(Queue *q);
int    q_push(Queue *q, void *item);   /* O(1); 0 = ok, -1 = out of memory */
void  *q_pop(Queue *q);                /* O(1); NULL if empty */
size_t q_size(const Queue *q);
void   q_free(Queue *q);               /* frees nodes only, not items */
#ifdef __cplusplus
}
#endif
#endif
