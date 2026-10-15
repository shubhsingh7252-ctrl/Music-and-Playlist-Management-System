/* avl.h - self-balancing binary search tree (DSA: AVL Tree). Keeps library sorted by title. */
#ifndef AVL_H
#define AVL_H
typedef struct AvlNode { char *key; void *val; int h; struct AvlNode *l, *r; } AvlNode;

AvlNode *avl_insert(AvlNode *n, const char *key, void *val);
AvlNode *avl_remove(AvlNode *n, const char *key);
void    *avl_find(AvlNode *n, const char *key);
void     avl_inorder(AvlNode *n, void (*fn)(void *val, void *ctx), void *ctx);
int      avl_height(AvlNode *n);
int      avl_is_balanced(AvlNode *n);
void     avl_free(AvlNode *n);
#endif
