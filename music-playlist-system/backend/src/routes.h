#ifndef ROUTES_H
#define ROUTES_H
#include "http.h"
#include "util.h"

typedef struct { int status; SB body; } ApiRes;
/* Handles every /api/ endpoint except media streaming. Caller holds the store lock. */
void api_handle(Request *rq, ApiRes *res);
#endif
