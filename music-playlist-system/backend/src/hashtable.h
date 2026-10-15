/* hashtable.h - separate-chaining hash table, string keys, auto-rehash (DSA: Hashing) */
#ifndef HASHTABLE_H
#define HASHTABLE_H
#include <stddef.h>
typedef struct HNode { char *key; void *val; struct HNode *next; } HNode;
typedef struct { HNode **b; size_t nb, count; } HashTable;
typedef void (*ht_visit)(const char *key, void *val, void *ctx);

HashTable *ht_new(size_t buckets);
void  ht_put(HashTable *t, const char *key, void *val);   /* insert or replace */
void *ht_get(HashTable *t, const char *key);
void *ht_remove(HashTable *t, const char *key);           /* returns removed value */
void  ht_foreach(HashTable *t, ht_visit fn, void *ctx);
void  ht_free(HashTable *t, void (*vfree)(void *));
#endif
