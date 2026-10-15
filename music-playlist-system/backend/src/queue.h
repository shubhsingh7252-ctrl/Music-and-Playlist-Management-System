/* queue.h - play queue: FIFO enqueue + cursor (next/prev) over a doubly linked list (DSA: Queue) */
#ifndef QUEUE_H
#define QUEUE_H
#include "dlist.h"
typedef struct { DList items; DNode *cur; } PlayQueue;

void pq_init(PlayQueue *q);
void pq_clear(PlayQueue *q);
void pq_enqueue(PlayQueue *q, int id);              /* append at back; becomes current if queue was empty */
void pq_set(PlayQueue *q, const int *ids, int n, int index);
int  pq_current(PlayQueue *q);                      /* -1 if none */
int  pq_index(PlayQueue *q);
int  pq_next(PlayQueue *q);                         /* 1 if moved */
int  pq_prev(PlayQueue *q);
int  pq_remove_at(PlayQueue *q, int index);
void pq_remove_val(PlayQueue *q, int id);
#endif
