/* heap.h - binary max-heap (DSA: Heap / Priority Queue). Top-N plays and recommendation ranking. */
#ifndef HEAP_H
#define HEAP_H
typedef struct { int id; int key; } HeapItem;
typedef struct { HeapItem *a; int n, cap; } MaxHeap;

void heap_init(MaxHeap *h);
void heap_push(MaxHeap *h, int id, int key);
int  heap_pop(MaxHeap *h, HeapItem *out);     /* 1 on success */
void heap_free(MaxHeap *h);
#endif
