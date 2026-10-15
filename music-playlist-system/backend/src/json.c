#include "json.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static const char *ws(const char *p) { while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++; return p; }

static const char *find_key(const char *js, const char *key) {
    size_t kl = strlen(key);
    const char *p = js;
    while (*p) {
        if (*p == '"') {
            const char *s = p + 1, *q = s;
            while (*q && *q != '"') { if (*q == '\\' && q[1]) q++; q++; }
            if (!*q) return NULL;
            const char *after = ws(q + 1);
            if (*after == ':' && (size_t)(q - s) == kl && strncmp(s, key, kl) == 0) return ws(after + 1);
            p = q + 1;
        } else p++;
    }
    return NULL;
}
static int put_utf8(char *o, unsigned cp) {
    if (cp < 0x80) { o[0] = (char)cp; return 1; }
    if (cp < 0x800) { o[0] = (char)(0xC0 | cp >> 6); o[1] = (char)(0x80 | (cp & 63)); return 2; }
    o[0] = (char)(0xE0 | cp >> 12); o[1] = (char)(0x80 | ((cp >> 6) & 63)); o[2] = (char)(0x80 | (cp & 63)); return 3;
}
int json_str(const char *js, const char *key, char *out, size_t cap) {
    const char *v = find_key(js, key);
    if (!v || *v != '"' || !cap) return 0;
    v++;
    size_t o = 0;
    while (*v && *v != '"') {
        char buf[4]; int n = 1;
        if (*v == '\\') {
            v++;
            switch (*v) {
                case 'n': buf[0] = '\n'; break;
                case 't': buf[0] = '\t'; break;
                case 'r': buf[0] = '\r'; break;
                case 'u': {
                    char h[5] = {0}; strncpy(h, v + 1, 4);
                    n = put_utf8(buf, (unsigned)strtoul(h, NULL, 16));
                    v += 4; break;
                }
                default: buf[0] = *v;
            }
            if (*v) v++;
        } else buf[0] = *v++;
        if (o + (size_t)n >= cap) break;
        memcpy(out + o, buf, (size_t)n); o += (size_t)n;
    }
    out[o] = 0;
    return 1;
}
int json_int(const char *js, const char *key, int *out) {
    const char *v = find_key(js, key);
    if (!v || (!isdigit((unsigned char)*v) && *v != '-')) return 0;
    *out = (int)strtol(v, NULL, 10);
    return 1;
}
int json_int_array(const char *js, const char *key, int *out, int max) {
    const char *v = find_key(js, key);
    if (!v || *v != '[') return -1;
    v++;
    int n = 0;
    for (;;) {
        v = ws(v);
        if (*v == ']' || !*v) break;
        if (*v == ',') { v++; continue; }
        char *e;
        long x = strtol(v, &e, 10);
        if (e == v) return -1;
        if (n < max) out[n++] = (int)x;
        v = e;
    }
    return n;
}
