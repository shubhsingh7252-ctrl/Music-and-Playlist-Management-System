/* http.h - small HTTP/1.1 server helpers on top of raw sockets */
#ifndef HTTP_H
#define HTTP_H
#include <stdio.h>
#include "platform.h"

#define MAX_BODY (60u * 1024u * 1024u)

typedef struct {
    char method[8];
    char path[512];
    char query[1024];
    char auth[200];
    char range[80];
    char ctype[80];
    size_t content_length;
    unsigned char *body;          /* NUL terminated, owned by request */
    size_t body_len;
} Request;

int  http_read(sock_t s, Request *rq);   /* 0 ok, -1 closed, -2 too large, -3 malformed */
void http_free(Request *rq);
int  http_send(sock_t s, int status, const char *ctype, const void *body, size_t len);
void http_error(sock_t s, int status, const char *msg);
void http_options(sock_t s);
void http_static(sock_t s, Request *rq, const char *root);
void http_stream(sock_t s, Request *rq, const unsigned char *mem, FILE *fp, size_t total, const char *ctype);
#endif
