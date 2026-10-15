/* Unit tests for every data structure. Run: make test */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "hashtable.h"
#include "dlist.h"
#include "queue.h"
#include "stack.h"
#include "avl.h"
#include "trie.h"
#include "heap.h"
#include "sort.h"
#include "graph.h"
#include "sha256.h"

static int passed = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); exit(1); } passed++; } while (0)

static int cmp_int(const void *a, const void *b) { return *(const int *)a - *(const int *)b; }

int main(void) {
    /* hash table */
    HashTable *h = ht_new(4);
    char k[16]; static int vals[500];
    for (int i = 0; i < 500; i++) { vals[i] = i; sprintf(k, "key%d", i); ht_put(h, k, &vals[i]); }
    CHECK(h->count == 500);
    CHECK(*(int *)ht_get(h, "key321") == 321);
    CHECK(ht_remove(h, "key321") != NULL && ht_get(h, "key321") == NULL && h->count == 499);
    ht_free(h, NULL);

    /* doubly linked list */
    DList l; dl_init(&l);
    for (int i = 0; i < 5; i++) dl_push_back(&l, i, 0);          /* 0 1 2 3 4 */
    CHECK(dl_move(&l, 0, 3) == 1);                                /* 1 2 3 0 4 */
    CHECK(dl_at(&l, 3)->val == 0 && l.head->val == 1);
    CHECK(dl_move(&l, 4, 0) == 1);                                /* 4 1 2 3 0 */
    CHECK(l.head->val == 4 && l.tail->val == 0 && l.size == 5);
    CHECK(dl_remove_val(&l, 2) == 1 && l.size == 4 && dl_find(&l, 2) == NULL);
    CHECK(dl_move(&l, 9, 0) == 0);
    dl_clear(&l);

    /* queue */
    PlayQueue q; pq_init(&q);
    pq_enqueue(&q, 10); pq_enqueue(&q, 20); pq_enqueue(&q, 30);
    CHECK(pq_current(&q) == 10 && pq_next(&q) && pq_current(&q) == 20);
    CHECK(pq_next(&q) && !pq_next(&q) && pq_current(&q) == 30);
    CHECK(pq_prev(&q) && pq_current(&q) == 20 && pq_index(&q) == 1);
    pq_remove_val(&q, 20); CHECK(pq_current(&q) == 30 && q.items.size == 2);
    int ids[3] = {1, 2, 3}; pq_set(&q, ids, 3, 2); CHECK(pq_current(&q) == 3);
    pq_clear(&q);

    /* stack */
    Stack s; stack_init(&s, 3); int v;
    for (int i = 1; i <= 5; i++) stack_push(&s, i);               /* keeps 5 4 3 */
    CHECK(s.size == 3 && stack_pop(&s, &v) && v == 5 && stack_pop(&s, &v) && v == 4);
    CHECK(stack_peek(&s, &v) && v == 3 && stack_pop(&s, &v) && !stack_pop(&s, &v));

    /* AVL: sorted order + balance after 1000 inserts and 500 deletes */
    AvlNode *t = NULL; char kk[16];
    for (int i = 0; i < 1000; i++) { sprintf(kk, "%06d", i); t = avl_insert(t, kk, &vals[i % 500]); }
    CHECK(avl_is_balanced(t) && avl_height(t) <= 12);              /* log2(1000) ~ 10 */
    for (int i = 0; i < 1000; i += 2) { sprintf(kk, "%06d", i); t = avl_remove(t, kk); }
    CHECK(avl_is_balanced(t) && avl_find(t, "000001") && !avl_find(t, "000002"));
    avl_free(t);

    /* trie */
    TrieNode *tr = trie_new(); int out[10];
    trie_add(tr, "neon", 1); trie_add(tr, "night", 2); trie_add(tr, "nova", 3); trie_add(tr, "neon", 1);
    CHECK(trie_prefix(tr, "ne", out, 10) == 1 && out[0] == 1);
    CHECK(trie_prefix(tr, "n", out, 10) == 3);
    trie_del(tr, "nova", 3); CHECK(trie_prefix(tr, "no", out, 10) == 0);
    CHECK(trie_prefix(tr, "zzz", out, 10) == 0);
    trie_free(tr);

    /* heap */
    MaxHeap hp; heap_init(&hp); HeapItem it;
    int keys[] = {5, 1, 9, 3, 7};
    for (int i = 0; i < 5; i++) heap_push(&hp, i, keys[i]);
    heap_pop(&hp, &it); CHECK(it.key == 9);
    heap_pop(&hp, &it); CHECK(it.key == 7);
    heap_pop(&hp, &it); CHECK(it.key == 5);
    heap_free(&hp);

    /* merge sort */
    int arr[200]; int *pa[200];
    srand(7);
    for (int i = 0; i < 200; i++) { arr[i] = rand() % 1000; pa[i] = &arr[i]; }
    merge_sort((void **)pa, 200, cmp_int);
    for (int i = 1; i < 200; i++) CHECK(*pa[i - 1] <= *pa[i]);

    /* graph + BFS recommendation */
    GNode nodes[4] = { {1, 1, "Pop", "Happy"}, {2, 1, "Pop", "Calm"}, {3, 2, "Pop", "Happy"}, {4, 3, "Rock", "Sad"} };
    Graph *g = graph_build(nodes, 4);
    int rec[4]; int n = graph_recommend(g, 1, rec, 3);
    CHECK(n >= 2 && rec[0] == 2);                                  /* same artist+genre beats same genre+mood */
    CHECK(graph_recommend(g, 4, rec, 3) == 0 || rec[0] != 4);
    graph_free(g);

    /* SHA-256 known vectors */
    char hex[65];
    sha256_hex("abc", 3, hex);
    CHECK(!strcmp(hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
    sha256_hex("", 0, hex);
    CHECK(!strcmp(hex, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));

    printf("All %d checks passed.\n", passed);
    return 0;
}
