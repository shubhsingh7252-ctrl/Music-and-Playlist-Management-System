#include "stack.h"
#include <stdlib.h>

void stack_init(Stack *s, int cap) { s->top = NULL; s->size = 0; s->cap = cap; }
void stack_clear(Stack *s) {
    SNode *n = s->top;
    while (n) { SNode *nx = n->next; free(n); n = nx; }
    s->top = NULL; s->size = 0;
}
void stack_push(Stack *s, int v) {
    SNode *n = malloc(sizeof *n);
    n->val = v; n->next = s->top; s->top = n; s->size++;
    if (s->size > s->cap) {                       /* evict bottom element */
        SNode *p = s->top;
        while (p->next && p->next->next) p = p->next;
        free(p->next); p->next = NULL; s->size--;
    }
}
int stack_pop(Stack *s, int *out) {
    if (!s->top) return 0;
    SNode *n = s->top;
    if (out) *out = n->val;
    s->top = n->next; free(n); s->size--;
    return 1;
}
int stack_peek(Stack *s, int *out) { if (!s->top) return 0; *out = s->top->val; return 1; }
void stack_remove_val(Stack *s, int v) {
    SNode **pp = &s->top;
    while (*pp) {
        if ((*pp)->val == v) { SNode *n = *pp; *pp = n->next; free(n); s->size--; }
        else pp = &(*pp)->next;
    }
}
