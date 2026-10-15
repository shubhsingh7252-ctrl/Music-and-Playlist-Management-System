/* Music & Playlist Management System - backend server (pure C, DSA project) */
#include "platform.h"
#include "http.h"
#include "routes.h"
#include "store.h"
#include "audio.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>

static char webroot[400] = "frontend";

static const char *audio_mime(const char *file) {
    const char *e = strrchr(file, '.');
    if (e && !strcmp(e, ".mp3")) return "audio/mpeg";
    if (e && !strcmp(e, ".flac")) return "audio/flac";
    return "audio/wav";
}
/* GET /api/songs/<id>/stream - public so <audio src> works; supports Range for seeking */
static int try_stream(sock_t s, Request *rq) {
    int id;
    if (strcmp(rq->method, "GET") && strcmp(rq->method, "HEAD")) return 0;
    if (sscanf(rq->path, "/api/songs/%d/stream", &id) != 1 || !strstr(rq->path, "/stream")) return 0;
    char file[80] = "", path[400]; int seed = 0, found = 0;
    store_lock();
    Song *song = song_by_id(id);
    if (song) { found = 1; seed = song->seed; str_copy(file, sizeof file, song->file); }
    snprintf(path, sizeof path, "%s/%s", store_upload_dir(), file);
    store_unlock();
    if (!found) { http_error(s, 404, "Song not found"); return 1; }
    if (!file[0]) {
        size_t len; const unsigned char *wav = audio_demo_wav(seed, &len);
        http_stream(s, rq, wav, NULL, len, "audio/wav");
    } else {
        FILE *f = fopen(path, "rb");
        if (!f) { http_error(s, 404, "Audio file missing"); return 1; }
        fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
        http_stream(s, rq, NULL, f, (size_t)sz, audio_mime(file));
        fclose(f);
    }
    return 1;
}

static void worker(void *arg) {
    sock_t s = *(sock_t *)arg; free(arg);
    sock_timeout(s, 30);
    Request rq;
    int rc = http_read(s, &rq);
    if (rc == -2) http_error(s, 413, "File bahut badi hai (max 60 MB)");
    else if (rc == -3) http_error(s, 400, "Bad request");
    if (rc == 0) {
        printf("%s %s\n", rq.method, rq.path); fflush(stdout);
        if (!strcmp(rq.method, "OPTIONS")) http_options(s);
        else if (try_stream(s, &rq)) { /* handled */ }
        else if (!strncmp(rq.path, "/api/", 5)) {
            ApiRes res;
            store_lock(); g_requests++; api_handle(&rq, &res); store_unlock();
            http_send(s, res.status, "application/json", res.body.d, res.body.len);
            sb_free(&res.body);
        } else http_static(s, &rq, webroot);
        http_free(&rq);
    }
    sock_close_(s);
}

static int has_index(const char *dir) {
    char p[450]; snprintf(p, sizeof p, "%s/index.html", dir);
    FILE *f = fopen(p, "rb"); if (f) fclose(f);
    return f != NULL;
}

int main(int argc, char **argv) {
    int port = getenv("PORT") ? atoi(getenv("PORT")) : 8080;
    const char *data = "data", *web = NULL;
    for (int i = 1; i + 1 < argc; i += 2) {
        if (!strcmp(argv[i], "-p")) port = atoi(argv[i + 1]);
        else if (!strcmp(argv[i], "-d")) data = argv[i + 1];
        else if (!strcmp(argv[i], "-w")) web = argv[i + 1];
    }
    if (web) str_copy(webroot, sizeof webroot, web);
    else {
        const char *cands[] = { "frontend", "../frontend", "../../frontend" };
        for (int i = 0; i < 3; i++) if (has_index(cands[i])) { str_copy(webroot, sizeof webroot, cands[i]); break; }
    }
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif
    if (net_init() != 0) { fprintf(stderr, "network init failed\n"); return 1; }
    store_init(data);

    sock_t ls = socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1; setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof yes);
    struct sockaddr_in addr; memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET; addr.sin_addr.s_addr = htonl(INADDR_ANY); addr.sin_port = htons((unsigned short)port);
    if (bind(ls, (struct sockaddr *)&addr, sizeof addr) != 0 || listen(ls, 64) != 0) {
        fprintf(stderr, "Port %d bind/listen fail - koi aur program use kar raha hoga. Try: -p 9000\n", port);
        return 1;
    }
    printf("==============================================\n");
    printf(" Music & Playlist Management System (C backend)\n");
    printf(" Open: http://localhost:%d\n", port);
    printf(" Web root: %s   Data dir: %s\n", webroot, data);
    printf("==============================================\n");
    for (;;) {
        sock_t c = accept(ls, NULL, NULL);
        if (c == SOCK_BAD) continue;
        sock_t *p = malloc(sizeof *p); *p = c;
        if (spawn_detached(worker, p)) { free(p); sock_close_(c); }
    }
}
