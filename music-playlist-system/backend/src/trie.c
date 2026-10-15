#include "trie.h"
#include <stdlib.h>
#include <ctype.h>

static int idx(char c) {
    c = (char)tolower((unsigned char)c);
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c >= '0' && c <= '9') return 26 + (c - '0');
    return -1;
}
TrieNode *trie_new(void) { return calloc(1, sizeof(TrieNode)); }
static int has(TrieNode *n, int id) { for (int i = 0; i < n->n; i++) if (n->ids[i] == id) return 1; return 0; }
void trie_add(TrieNode *root, const char *w, int id) {
    TrieNode *n = root;
    for (; *w; w++) {
        int i = idx(*w);
        if (i < 0) continue;
        if (!n->ch[i]) n->ch[i] = trie_new();
        n = n->ch[i];
    }
    if (n == root || has(n, id)) return;
    if (n->n == n->cap) { n->cap = n->cap ? n->cap * 2 : 2; n->ids = realloc(n->ids, sizeof(int) * (size_t)n->cap); }
    n->ids[n->n++] = id;
}
void trie_del(TrieNode *root, const char *w, int id) {
    TrieNode *n = root;
    for (; *w && n; w++) { int i = idx(*w); if (i < 0) continue; n = n->ch[i]; }
    if (!n || n == root) return;
    for (int i = 0; i < n->n; i++)
        if (n->ids[i] == id) { n->ids[i] = n->ids[--n->n]; return; }
}
static void collect(TrieNode *n, int *out, int *cnt, int max) {
    if (!n || *cnt >= max) return;
    for (int i = 0; i < n->n && *cnt < max; i++) {
        int dup = 0;
        for (int j = 0; j < *cnt; j++) if (out[j] == n->ids[i]) { dup = 1; break; }
        if (!dup) out[(*cnt)++] = n->ids[i];
    }
    for (int c = 0; c < 36; c++) collect(n->ch[c], out, cnt, max);
}
int trie_prefix(TrieNode *root, const char *p, int *out, int max) {
    TrieNode *n = root;
    for (; *p && n; p++) { int i = idx(*p); if (i < 0) continue; n = n->ch[i]; }
    int cnt = 0;
    if (n) collect(n, out, &cnt, max);
    return cnt;
}
void trie_free(TrieNode *n) {
    if (!n) return;
    for (int i = 0; i < 36; i++) trie_free(n->ch[i]);
    free(n->ids); free(n);
}
