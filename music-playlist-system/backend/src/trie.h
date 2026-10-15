/* trie.h - prefix tree for instant search suggestions (DSA: Trie) */
#ifndef TRIE_H
#define TRIE_H
typedef struct TrieNode { struct TrieNode *ch[36]; int *ids; int n, cap; } TrieNode;

TrieNode *trie_new(void);
void trie_add(TrieNode *root, const char *word, int id);
void trie_del(TrieNode *root, const char *word, int id);
int  trie_prefix(TrieNode *root, const char *prefix, int *out, int max);   /* unique ids */
void trie_free(TrieNode *root);
#endif
