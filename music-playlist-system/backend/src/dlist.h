/* dlist.h - doubly linked list of ints (DSA: Linked List). Used for playlist song order. */
#ifndef DLIST_H
#define DLIST_H
#include <time.h>
typedef struct DNode { int val; long added; struct DNode *prev, *next; } DNode;
typedef struct { DNode *head, *tail; int size; } DList;

void   dl_init(DList *l);
void   dl_clear(DList *l);
DNode *dl_push_back(DList *l, int val, long added);
DNode *dl_find(DList *l, int val);
DNode *dl_at(DList *l, int idx);
int    dl_index_of(DList *l, DNode *n);
void   dl_remove_node(DList *l, DNode *n);
int    dl_remove_val(DList *l, int val);
int    dl_move(DList *l, int from, int to);     /* drag & drop reorder; 1 on success */
#endif
