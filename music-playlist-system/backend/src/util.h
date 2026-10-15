/* util.h - string builder (JSON output) and small string helpers */
#ifndef UTIL_H
#define UTIL_H
#include <stddef.h>

typedef struct { char *d; size_t len, cap; } SB;
void sb_init(SB *s);
void sb_free(SB *s);
void sb_addn(SB *s, const char *p, size_t n);
void sb_add(SB *s, const char *p);
void sb_fmt(SB *s, const char *fmt, ...);
void sb_json(SB *s, const char *str);      /* appends "quoted \"escaped\" string" */

char *str_dup(const char *s);
void  str_copy(char *dst, size_t cap, const char *src);
void  str_lower(char *s);
void  str_clean(char *s);                  /* control chars -> space, trim */
int   ci_eq(const char *a, const char *b);
int   ci_contains(const char *hay, const char *needle);
void  url_decode(char *s);
int   qs_get(const char *query, const char *key, char *out, size_t cap);
void  rand_hex(char *out, size_t nbytes);  /* writes 2*nbytes hex chars + NUL */
#endif
