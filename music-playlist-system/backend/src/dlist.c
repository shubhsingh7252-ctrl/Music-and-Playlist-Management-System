#include "dlist.h"
#include <stdlib.h>

void dl_init(DList *l) { l->head = l->tail = NULL; l->size = 0; }
void dl_clear(DList *l) {
    DNode *n = l->head;
    while (n) { DNode *nx = n->next; free(n); n = nx; }
    dl_init(l);
}
DNode *dl_push_back(DList *l, int val, long added) {
    DNode *n = malloc(sizeof *n);
    n->val = val; n->added = added; n->next = NULL; n->prev = l->tail;
    if (l->tail) l->tail->next = n; else l->head = n;
    l->tail = n; l->size++;
    return n;
}
DNode *dl_find(DList *l, int val) {
    for (DNode *n = l->head; n; n = n->next) if (n->val == val) return n;
    return NULL;
}
DNode *dl_at(DList *l, int idx) {
    if (idx < 0 || idx >= l->size) return NULL;
    DNode *n;
    if (idx < l->size / 2) { n = l->head; while (idx--) n = n->next; }
    else { n = l->tail; for (int i = l->size - 1; i > idx; i--) n = n->prev; }
    return n;
}
int dl_index_of(DList *l, DNode *t) {
    int i = 0;
    for (DNode *n = l->head; n; n = n->next, i++) if (n == t) return i;
    return -1;
}
void dl_remove_node(DList *l, DNode *n) {
    if (n->prev) n->prev->next = n->next; else l->head = n->next;
    if (n->next) n->next->prev = n->prev; else l->tail = n->prev;
    free(n); l->size--;
}
int dl_remove_val(DList *l, int val) {
    DNode *n = dl_find(l, val);
    if (!n) return 0;
    dl_remove_node(l, n);
    return 1;
}
int dl_move(DList *l, int from, int to) {
    if (from == to || from < 0 || to < 0 || from >= l->size || to >= l->size) return 0;
    DNode *n = dl_at(l, from);
    /* unlink without freeing */
    if (n->prev) n->prev->next = n->next; else l->head = n->next;
    if (n->next) n->next->prev = n->prev; else l->tail = n->prev;
    l->size--;
    if (to == 0) {
        n->prev = NULL; n->next = l->head;
        if (l->head) l->head->prev = n; else l->tail = n;
        l->head = n;
    } else {
        DNode *after = dl_at(l, to - 1);
        n->prev = after; n->next = after->next;
        if (after->next) after->next->prev = n; else l->tail = n;
        after->next = n;
    }
    l->size++;
    return 1;
}
