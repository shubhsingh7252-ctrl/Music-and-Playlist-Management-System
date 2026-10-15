/* stack.h - bounded linked stack for "recently played" history (DSA: Stack, LIFO) */
#ifndef STACK_H
#define STACK_H
typedef struct SNode { int val; struct SNode *next; } SNode;
typedef struct { SNode *top; int size, cap; } Stack;

void stack_init(Stack *s, int cap);
void stack_clear(Stack *s);
void stack_push(Stack *s, int v);        /* drops the oldest entry when full */
int  stack_pop(Stack *s, int *out);      /* 1 on success */
int  stack_peek(Stack *s, int *out);
void stack_remove_val(Stack *s, int v);
#endif
