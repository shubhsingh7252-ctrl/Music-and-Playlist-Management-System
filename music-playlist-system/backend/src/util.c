#include "util.h"
#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

void sb_init(SB *s) { s->cap = 256; s->len = 0; s->d = malloc(s->cap); s->d[0] = 0; }
void sb_free(SB *s) { free(s->d); s->d = NULL; s->len = s->cap = 0; }
void sb_addn(SB *s, const char *p, size_t n) {
    if (s->len + n + 1 > s->cap) {
        while (s->len + n + 1 > s->cap) s->cap *= 2;
        s->d = realloc(s->d, s->cap);
    }
    memcpy(s->d + s->len, p, n);
    s->len += n;
    s->d[s->len] = 0;
}
void sb_add(SB *s, const char *p) { sb_addn(s, p, strlen(p)); }
void sb_fmt(SB *s, const char *fmt, ...) {
    va_list ap, ap2;
    va_start(ap, fmt);
    va_copy(ap2, ap);
    char probe[1]; int n = vsnprintf(probe, 1, fmt, ap);
    va_end(ap);
    if (n > 0) {
        char *tmp = malloc((size_t)n + 1);
        vsnprintf(tmp, (size_t)n + 1, fmt, ap2);
        sb_addn(s, tmp, (size_t)n);
        free(tmp);
    }
    va_end(ap2);
}
void sb_json(SB *s, const char *str) {
    sb_addn(s, "\"", 1);
    for (const unsigned char *p = (const unsigned char *)str; *p; p++) {
        switch (*p) {
            case '"':  sb_add(s, "\\\""); break;
            case '\\': sb_add(s, "\\\\"); break;
            case '\n': sb_add(s, "\\n"); break;
            case '\r': sb_add(s, "\\r"); break;
            case '\t': sb_add(s, "\\t"); break;
            default:
                if (*p < 0x20) sb_fmt(s, "\\u%04x", *p);
                else sb_addn(s, (const char *)p, 1);
        }
    }
    sb_addn(s, "\"", 1);
}

char *str_dup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}
void str_copy(char *dst, size_t cap, const char *src) {
    if (!cap) return;
    size_t n = strlen(src);
    if (n >= cap) n = cap - 1;
    memcpy(dst, src, n);
    dst[n] = 0;
}
void str_lower(char *s) { for (; *s; s++) *s = (char)tolower((unsigned char)*s); }
void str_clean(char *s) {
    for (char *p = s; *p; p++) if ((unsigned char)*p < 0x20) *p = ' ';
    size_t n = strlen(s);
    while (n && s[n - 1] == ' ') s[--n] = 0;
    char *a = s;
    while (*a == ' ') a++;
    if (a != s) memmove(s, a, strlen(a) + 1);
}
int ci_eq(const char *a, const char *b) {
    for (; *a && *b; a++, b++)
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
    return *a == *b;
}
int ci_contains(const char *hay, const char *needle) {
    size_t nl = strlen(needle);
    if (!nl) return 1;
    for (; *hay; hay++) {
        size_t i = 0;
        while (i < nl && hay[i] && tolower((unsigned char)hay[i]) == tolower((unsigned char)needle[i])) i++;
        if (i == nl) return 1;
    }
    return 0;
}
static int hexv(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
void url_decode(char *s) {
    char *o = s;
    while (*s) {
        if (*s == '%' && hexv(s[1]) >= 0 && hexv(s[2]) >= 0) { *o++ = (char)(hexv(s[1]) * 16 + hexv(s[2])); s += 3; }
        else if (*s == '+') { *o++ = ' '; s++; }
        else *o++ = *s++;
    }
    *o = 0;
}
int qs_get(const char *query, const char *key, char *out, size_t cap) {
    size_t kl = strlen(key);
    const char *p = query;
    while (*p) {
        const char *e = strchr(p, '&');
        size_t len = e ? (size_t)(e - p) : strlen(p);
        if (len > kl && p[kl] == '=' && strncmp(p, key, kl) == 0) {
            size_t vl = len - kl - 1;
            if (vl >= cap) vl = cap - 1;
            memcpy(out, p + kl + 1, vl);
            out[vl] = 0;
            url_decode(out);
            return 1;
        }
        if (!e) break;
        p = e + 1;
    }
    return 0;
}
void rand_hex(char *out, size_t nbytes) {
    unsigned char buf[64];
    if (nbytes > sizeof buf) nbytes = sizeof buf;
    rand_bytes(buf, nbytes);
    for (size_t i = 0; i < nbytes; i++) sprintf(out + i * 2, "%02x", buf[i]);
    out[nbytes * 2] = 0;
}
