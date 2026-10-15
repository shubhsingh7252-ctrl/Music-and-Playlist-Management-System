#include "queue.h"

void pq_init(PlayQueue *q) { dl_init(&q->items); q->cur = NULL; }
void pq_clear(PlayQueue *q) { dl_clear(&q->items); q->cur = NULL; }
void pq_enqueue(PlayQueue *q, int id) {
    DNode *n = dl_push_back(&q->items, id, 0);
    if (!q->cur) q->cur = n;
}
void pq_set(PlayQueue *q, const int *ids, int n, int index) {
    pq_clear(q);
    for (int i = 0; i < n; i++) dl_push_back(&q->items, ids[i], 0);
    q->cur = dl_at(&q->items, index < 0 ? 0 : index);
    if (!q->cur) q->cur = q->items.head;
}
int pq_current(PlayQueue *q) { return q->cur ? q->cur->val : -1; }
int pq_index(PlayQueue *q) { return q->cur ? dl_index_of(&q->items, q->cur) : -1; }
int pq_next(PlayQueue *q) { if (q->cur && q->cur->next) { q->cur = q->cur->next; return 1; } return 0; }
int pq_prev(PlayQueue *q) { if (q->cur && q->cur->prev) { q->cur = q->cur->prev; return 1; } return 0; }
int pq_remove_at(PlayQueue *q, int index) {
    DNode *n = dl_at(&q->items, index);
    if (!n) return 0;
    if (q->cur == n) q->cur = n->next ? n->next : n->prev;
    dl_remove_node(&q->items, n);
    return 1;
}
void pq_remove_val(PlayQueue *q, int id) {
    DNode *n;
    while ((n = dl_find(&q->items, id))) {
        if (q->cur == n) q->cur = n->next ? n->next : n->prev;
        dl_remove_node(&q->items, n);
    }
}
