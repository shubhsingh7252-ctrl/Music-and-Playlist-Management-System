#include "hashtable.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

static unsigned long hfn(const char *s) {               /* djb2 */
    unsigned long h = 5381; int c;
    while ((c = (unsigned char)*s++)) h = ((h << 5) + h) + (unsigned long)c;
    return h;
}
HashTable *ht_new(size_t nb) {
    HashTable *t = calloc(1, sizeof *t);
    t->nb = nb ? nb : 16;
    t->b = calloc(t->nb, sizeof(HNode *));
    return t;
}
static void rehash(HashTable *t) {
    size_t nn = t->nb * 2;
    HNode **nb = calloc(nn, sizeof(HNode *));
    for (size_t i = 0; i < t->nb; i++) {
        HNode *n = t->b[i];
        while (n) { HNode *nx = n->next; size_t j = hfn(n->key) % nn; n->next = nb[j]; nb[j] = n; n = nx; }
    }
    free(t->b); t->b = nb; t->nb = nn;
}
void ht_put(HashTable *t, const char *key, void *val) {
    size_t i = hfn(key) % t->nb;
    for (HNode *n = t->b[i]; n; n = n->next)
        if (!strcmp(n->key, key)) { n->val = val; return; }
    HNode *n = malloc(sizeof *n);
    n->key = str_dup(key); n->val = val; n->next = t->b[i]; t->b[i] = n;
    t->count++;
    if (t->count * 4 > t->nb * 3) rehash(t);              /* load factor 0.75 */
}
void *ht_get(HashTable *t, const char *key) {
    for (HNode *n = t->b[hfn(key) % t->nb]; n; n = n->next)
        if (!strcmp(n->key, key)) return n->val;
    return NULL;
}
void *ht_remove(HashTable *t, const char *key) {
    HNode **pp = &t->b[hfn(key) % t->nb];
    while (*pp) {
        if (!strcmp((*pp)->key, key)) {
            HNode *n = *pp; void *v = n->val;
            *pp = n->next; free(n->key); free(n); t->count--;
            return v;
        }
        pp = &(*pp)->next;
    }
    return NULL;
}
void ht_foreach(HashTable *t, ht_visit fn, void *ctx) {
    for (size_t i = 0; i < t->nb; i++)
        for (HNode *n = t->b[i]; n; n = n->next) fn(n->key, n->val, ctx);
}
void ht_free(HashTable *t, void (*vfree)(void *)) {
    if (!t) return;
    for (size_t i = 0; i < t->nb; i++) {
        HNode *n = t->b[i];
        while (n) { HNode *nx = n->next; if (vfree) vfree(n->val); free(n->key); free(n); n = nx; }
    }
    free(t->b); free(t);
}
