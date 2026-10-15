#include "avl.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

int avl_height(AvlNode *n) { return n ? n->h : 0; }
static void upd(AvlNode *n) { int a = avl_height(n->l), b = avl_height(n->r); n->h = 1 + (a > b ? a : b); }
static AvlNode *rot_r(AvlNode *y) { AvlNode *x = y->l; y->l = x->r; x->r = y; upd(y); upd(x); return x; }
static AvlNode *rot_l(AvlNode *x) { AvlNode *y = x->r; x->r = y->l; y->l = x; upd(x); upd(y); return y; }
static AvlNode *rebalance(AvlNode *n) {
    upd(n);
    int bf = avl_height(n->l) - avl_height(n->r);
    if (bf > 1) {                                           /* left heavy */
        if (avl_height(n->l->l) < avl_height(n->l->r)) n->l = rot_l(n->l);   /* LR */
        return rot_r(n);                                    /* LL */
    }
    if (bf < -1) {                                          /* right heavy */
        if (avl_height(n->r->r) < avl_height(n->r->l)) n->r = rot_r(n->r);   /* RL */
        return rot_l(n);                                    /* RR */
    }
    return n;
}
AvlNode *avl_insert(AvlNode *n, const char *key, void *val) {
    if (!n) {
        AvlNode *x = calloc(1, sizeof *x);
        x->key = str_dup(key); x->val = val; x->h = 1;
        return x;
    }
    int c = strcmp(key, n->key);
    if (c < 0) n->l = avl_insert(n->l, key, val);
    else if (c > 0) n->r = avl_insert(n->r, key, val);
    else { n->val = val; return n; }
    return rebalance(n);
}
AvlNode *avl_remove(AvlNode *n, const char *key) {
    if (!n) return NULL;
    int c = strcmp(key, n->key);
    if (c < 0) n->l = avl_remove(n->l, key);
    else if (c > 0) n->r = avl_remove(n->r, key);
    else {
        if (!n->l || !n->r) {
            AvlNode *child = n->l ? n->l : n->r;
            free(n->key); free(n);
            return child;
        }
        AvlNode *s = n->r;                                  /* in-order successor */
        while (s->l) s = s->l;
        char *nk = str_dup(s->key); void *nv = s->val;
        n->r = avl_remove(n->r, nk);
        free(n->key); n->key = nk; n->val = nv;
    }
    return rebalance(n);
}
void *avl_find(AvlNode *n, const char *key) {
    while (n) {
        int c = strcmp(key, n->key);
        if (!c) return n->val;
        n = c < 0 ? n->l : n->r;
    }
    return NULL;
}
void avl_inorder(AvlNode *n, void (*fn)(void *, void *), void *ctx) {
    if (!n) return;
    avl_inorder(n->l, fn, ctx); fn(n->val, ctx); avl_inorder(n->r, fn, ctx);
}
int avl_is_balanced(AvlNode *n) {
    if (!n) return 1;
    int d = avl_height(n->l) - avl_height(n->r);
    return d >= -1 && d <= 1 && avl_is_balanced(n->l) && avl_is_balanced(n->r);
}
void avl_free(AvlNode *n) { if (!n) return; avl_free(n->l); avl_free(n->r); free(n->key); free(n); }
