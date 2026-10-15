#include "heap.h"
#include <stdlib.h>

void heap_init(MaxHeap *h) { h->a = NULL; h->n = h->cap = 0; }
void heap_free(MaxHeap *h) { free(h->a); heap_init(h); }
static void swap(HeapItem *x, HeapItem *y) { HeapItem t = *x; *x = *y; *y = t; }
void heap_push(MaxHeap *h, int id, int key) {
    if (h->n == h->cap) { h->cap = h->cap ? h->cap * 2 : 16; h->a = realloc(h->a, sizeof(HeapItem) * (size_t)h->cap); }
    int i = h->n++;
    h->a[i].id = id; h->a[i].key = key;
    while (i > 0 && h->a[(i - 1) / 2].key < h->a[i].key) { swap(&h->a[(i - 1) / 2], &h->a[i]); i = (i - 1) / 2; }
}
int heap_pop(MaxHeap *h, HeapItem *out) {
    if (!h->n) return 0;
    if (out) *out = h->a[0];
    h->a[0] = h->a[--h->n];
    int i = 0;
    for (;;) {
        int l = 2 * i + 1, r = l + 1, m = i;
        if (l < h->n && h->a[l].key > h->a[m].key) m = l;
        if (r < h->n && h->a[r].key > h->a[m].key) m = r;
        if (m == i) break;
        swap(&h->a[i], &h->a[m]); i = m;
    }
    return 1;
}
