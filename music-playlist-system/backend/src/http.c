#include "http.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

static const char *status_text(int c) {
    switch (c) {
        case 200: return "OK"; case 201: return "Created"; case 204: return "No Content";
        case 206: return "Partial Content"; case 400: return "Bad Request"; case 401: return "Unauthorized";
        case 403: return "Forbidden"; case 404: return "Not Found"; case 405: return "Method Not Allowed";
        case 409: return "Conflict"; case 413: return "Payload Too Large"; case 416: return "Range Not Satisfiable";
        default: return "Internal Server Error";
    }
}
static int send_all(sock_t s, const void *buf, size_t len) {
    const char *p = buf;
    while (len) {
        int n = (int)send(s, p, (int)(len > 65536 ? 65536 : len), MSG_NOSIGNAL);
        if (n <= 0) return -1;
        p += n; len -= (size_t)n;
    }
    return 0;
}
static int send_head(sock_t s, int status, const char *ctype, size_t len, const char *extra) {
    char h[768];
    int n = snprintf(h, sizeof h,
        "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %lu\r\nConnection: close\r\n"
        "Access-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: Authorization, Content-Type\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\nX-Content-Type-Options: nosniff\r\n%s\r\n",
        status, status_text(status), ctype, (unsigned long)len, extra ? extra : "");
    return send_all(s, h, (size_t)n);
}
int http_send(sock_t s, int status, const char *ctype, const void *body, size_t len) {
    if (send_head(s, status, ctype, len, NULL)) return -1;
    return len ? send_all(s, body, len) : 0;
}
void http_error(sock_t s, int status, const char *msg) {
    SB b; sb_init(&b); sb_add(&b, "{\"error\":"); sb_json(&b, msg); sb_add(&b, "}");
    http_send(s, status, "application/json", b.d, b.len);
    sb_free(&b);
}
void http_options(sock_t s) { send_head(s, 204, "text/plain", 0, "Access-Control-Max-Age: 600\r\n"); }

void http_free(Request *rq) { free(rq->body); rq->body = NULL; }

static int ieq_n(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; i++) if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i])) return 0;
    return 1;
}
int http_read(sock_t s, Request *rq) {
    char hdr[16384]; size_t have = 0; int hend = -1;
    memset(rq, 0, sizeof *rq);
    while (hend < 0) {
        if (have >= sizeof hdr - 1) return -3;
        int n = (int)recv(s, hdr + have, (int)(sizeof hdr - 1 - have), 0);
        if (n <= 0) return -1;
        have += (size_t)n; hdr[have] = 0;
        char *e = strstr(hdr, "\r\n\r\n");
        if (e) hend = (int)(e - hdr) + 4;
    }
    char target[1600];
    if (sscanf(hdr, "%7s %1599s HTTP/", rq->method, target) != 2) return -3;
    char *q = strchr(target, '?');
    if (q) { *q = 0; str_copy(rq->query, sizeof rq->query, q + 1); }
    str_copy(rq->path, sizeof rq->path, target); url_decode(rq->path);
    char *line = strchr(hdr, '\n');
    while (line && line + 1 < hdr + hend - 2) {
        line++;
        char *e = strchr(line, '\r');
        if (!e) break;
        *e = 0;
        char *c = strchr(line, ':');
        if (c) {
            size_t nl = (size_t)(c - line); char *v = c + 1;
            while (*v == ' ') v++;
            if (nl == 14 && ieq_n(line, "content-length", 14)) rq->content_length = (size_t)strtoull(v, NULL, 10);
            else if (nl == 13 && ieq_n(line, "authorization", 13)) str_copy(rq->auth, sizeof rq->auth, v);
            else if (nl == 5 && ieq_n(line, "range", 5)) str_copy(rq->range, sizeof rq->range, v);
            else if (nl == 12 && ieq_n(line, "content-type", 12)) str_copy(rq->ctype, sizeof rq->ctype, v);
        }
        line = e + 1;
    }
    if (rq->content_length > MAX_BODY) return -2;
    rq->body = malloc(rq->content_length + 1);
    size_t pre = have - (size_t)hend;
    if (pre > rq->content_length) pre = rq->content_length;
    memcpy(rq->body, hdr + hend, pre);
    rq->body_len = pre;
    while (rq->body_len < rq->content_length) {
        int n = (int)recv(s, (char *)rq->body + rq->body_len, (int)(rq->content_length - rq->body_len > 65536 ? 65536 : rq->content_length - rq->body_len), 0);
        if (n <= 0) { http_free(rq); return -1; }
        rq->body_len += (size_t)n;
    }
    rq->body[rq->body_len] = 0;
    return 0;
}

static const char *mime(const char *path) {
    const char *e = strrchr(path, '.');
    if (!e) return "application/octet-stream";
    if (!strcmp(e, ".html")) return "text/html; charset=utf-8";
    if (!strcmp(e, ".css")) return "text/css; charset=utf-8";
    if (!strcmp(e, ".js")) return "application/javascript; charset=utf-8";
    if (!strcmp(e, ".json")) return "application/json";
    if (!strcmp(e, ".svg")) return "image/svg+xml";
    if (!strcmp(e, ".png")) return "image/png";
    if (!strcmp(e, ".ico")) return "image/x-icon";
    if (!strcmp(e, ".mp3")) return "audio/mpeg";
    if (!strcmp(e, ".wav")) return "audio/wav";
    if (!strcmp(e, ".flac")) return "audio/flac";
    return "application/octet-stream";
}
void http_static(sock_t s, Request *rq, const char *root) {
    if (strcmp(rq->method, "GET") && strcmp(rq->method, "HEAD")) { http_error(s, 405, "Method not allowed"); return; }
    if (strstr(rq->path, "..") || strchr(rq->path, '\\')) { http_error(s, 403, "Forbidden"); return; }
    char path[900];
    snprintf(path, sizeof path, "%s%s%s", root, rq->path, rq->path[strlen(rq->path) - 1] == '/' ? "index.html" : "");
    FILE *f = fopen(path, "rb");
    if (!f) { http_error(s, 404, "Not found"); return; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)sz + 1);
    size_t got = fread(buf, 1, (size_t)sz, f); fclose(f);
    http_send(s, 200, mime(path), buf, got);
    free(buf);
}
void http_stream(sock_t s, Request *rq, const unsigned char *mem, FILE *fp, size_t total, const char *ctype) {
    long long start = 0, end = (long long)total - 1;
    int partial = 0;
    if (!strncmp(rq->range, "bytes=", 6)) {
        const char *p = rq->range + 6;
        if (*p == '-') { long long n = atoll(p + 1); start = (long long)total > n ? (long long)total - n : 0; }
        else { start = atoll(p); const char *d = strchr(p, '-'); if (d && d[1]) end = atoll(d + 1); }
        if (end >= (long long)total) end = (long long)total - 1;
        if (start > end || start >= (long long)total) {
            char ex[80]; snprintf(ex, sizeof ex, "Content-Range: bytes */%lu\r\n", (unsigned long)total);
            send_head(s, 416, "text/plain", 0, ex); return;
        }
        partial = 1;
    }
    size_t len = (size_t)(end - start + 1);
    char ex[160];
    if (partial) snprintf(ex, sizeof ex, "Accept-Ranges: bytes\r\nContent-Range: bytes %lld-%lld/%lu\r\n", start, end, (unsigned long)total);
    else snprintf(ex, sizeof ex, "Accept-Ranges: bytes\r\n");
    if (send_head(s, partial ? 206 : 200, ctype, len, ex)) return;
    if (!strcmp(rq->method, "HEAD")) return;
    if (mem) { send_all(s, mem + start, len); return; }
    fseek(fp, (long)start, SEEK_SET);
    unsigned char buf[65536];
    while (len) {
        size_t n = fread(buf, 1, len > sizeof buf ? sizeof buf : len, fp);
        if (!n || send_all(s, buf, n)) break;
        len -= n;
    }
}
