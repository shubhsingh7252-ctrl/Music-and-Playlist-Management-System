#include "routes.h"
#include "store.h"
#include "json.h"
#include "sort.h"
#include "heap.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define ERR(code, msg) do { res->status = (code); sb_add(&res->body, "{\"error\":"); sb_json(&res->body, msg); sb_add(&res->body, "}"); return; } while (0)
#define IS(m) (!strcmp(rq->method, (m)))

/* ---------- JSON writers ---------- */
static void song_json(SB *b, const Song *s) {
    sb_fmt(b, "{\"id\":%d,\"title\":", s->id); sb_json(b, s->title);
    sb_add(b, ",\"artist\":"); sb_json(b, artist_name(s->artist_id));
    sb_add(b, ",\"genre\":"); sb_json(b, s->genre);
    sb_add(b, ",\"mood\":"); sb_json(b, s->mood);
    sb_fmt(b, ",\"plays\":%d,\"duration\":%d,\"uploaded\":%s,\"uploader\":%d}", s->plays, s->duration, s->uploader ? "true" : "false", s->uploader);
}
static void user_json(SB *b, const User *u) {
    sb_fmt(b, "{\"id\":%d,\"username\":", u->id); sb_json(b, u->username);
    sb_add(b, ",\"email\":"); sb_json(b, u->email);
    sb_fmt(b, ",\"role\":\"%s\"}", u->admin ? "admin" : "user");
}
static void playlist_json(SB *b, Playlist *p) {
    sb_fmt(b, "{\"id\":%d,\"name\":", p->id); sb_json(b, p->name); sb_add(b, ",\"songs\":[");
    for (DNode *n = p->songs.head; n; n = n->next) sb_fmt(b, "%s%d", n == p->songs.head ? "" : ",", n->val);
    sb_add(b, "]}");
}
static void queue_json(SB *b, User *u) {
    sb_add(b, "{\"items\":[");
    for (DNode *n = u->queue.items.head; n; n = n->next) sb_fmt(b, "%s%d", n == u->queue.items.head ? "" : ",", n->val);
    sb_fmt(b, "],\"index\":%d}", pq_index(&u->queue));
}
static void songs_by_ids_json(SB *b, const int *ids, int n, const char *key) {
    sb_fmt(b, "{\"%s\":[", key);
    int first = 1;
    for (int i = 0; i < n; i++) {
        Song *s = song_by_id(ids[i]);
        if (!s) continue;
        if (!first) sb_add(b, ",");
        song_json(b, s); first = 0;
    }
    sb_add(b, "]}");
}

/* ---------- helpers ---------- */
static User *auth(Request *rq) {
    if (strncmp(rq->auth, "Bearer ", 7)) return NULL;
    return session_user(rq->auth + 7);
}
static int split_path(char *path, char **seg, int max) {
    int n = 0;
    for (char *p = strtok(path, "/"); p && n < max; p = strtok(NULL, "/")) seg[n++] = p;
    return n;
}
static int is_num(const char *s) { if (!*s) return 0; for (; *s; s++) if (!isdigit((unsigned char)*s)) return 0; return 1; }
static int valid_username(const char *u) {
    size_t n = strlen(u);
    if (n < 3 || n > 30) return 0;
    for (; *u; u++) if (!isalnum((unsigned char)*u) && *u != '_' && *u != '.' && *u != '-') return 0;
    return 1;
}
static int valid_email(const char *e) {
    const char *at = strchr(e, '@');
    return at && at != e && strchr(at, '.') && strchr(at, '.')[1] && !strchr(e, ' ');
}
static int cmp_artist(const void *a, const void *b) {
    const Song *x = a, *y = b;
    int c = strcmp(artist_name(x->artist_id), artist_name(y->artist_id));
    return c ? c : strcmp(x->title, y->title);
}
static int cmp_plays(const void *a, const void *b) { return ((const Song *)b)->plays - ((const Song *)a)->plays; }
static int cmp_newest(const void *a, const void *b) { return ((const Song *)b)->id - ((const Song *)a)->id; }

/* ---------- auth ---------- */
static void h_register(Request *rq, ApiRes *res) {
    char un[64] = "", em[128] = "", pw[128] = "";
    const char *js = (const char *)rq->body;
    json_str(js, "username", un, sizeof un); json_str(js, "email", em, sizeof em); json_str(js, "password", pw, sizeof pw);
    str_clean(un); str_clean(em);
    if (!valid_username(un)) ERR(400, "Username 3-30 chars (letters, digits, _ . -)");
    if (!valid_email(em)) ERR(400, "Valid email daalo");
    if (strlen(pw) < 4) ERR(400, "Password kam se kam 4 characters ka ho");
    User *u = user_register(un, em, pw);
    if (!u) ERR(409, "Username already taken");
    char tok[65]; session_new(u, tok); store_save();
    res->status = 201;
    sb_fmt(&res->body, "{\"token\":\"%s\",\"user\":", tok); user_json(&res->body, u); sb_add(&res->body, "}");
}
static void h_login(Request *rq, ApiRes *res) {
    char un[64] = "", pw[128] = "";
    json_str((const char *)rq->body, "username", un, sizeof un); json_str((const char *)rq->body, "password", pw, sizeof pw);
    User *u = user_login(un, pw);
    if (!u) ERR(401, "Wrong username/password");
    char tok[65]; session_new(u, tok);
    sb_fmt(&res->body, "{\"token\":\"%s\",\"user\":", tok); user_json(&res->body, u); sb_add(&res->body, "}");
}

/* ---------- songs ---------- */
static void h_songs_list(Request *rq, ApiRes *res) {
    char q[100] = "", genre[40] = "", sort[16] = "title";
    qs_get(rq->query, "q", q, sizeof q); qs_get(rq->query, "genre", genre, sizeof genre); qs_get(rq->query, "sort", sort, sizeof sort);
    int n; Song **a;
    if (!strcmp(sort, "title")) a = songs_by_title(&n);              /* AVL in-order: already sorted */
    else {
        a = songs_array(&n);
        merge_sort((void **)a, n, !strcmp(sort, "artist") ? cmp_artist : !strcmp(sort, "plays") ? cmp_plays : cmp_newest);
    }
    int all = !genre[0] || ci_eq(genre, "All"), cnt = 0;
    sb_add(&res->body, "{\"songs\":[");
    for (int i = 0; i < n; i++) {
        Song *s = a[i];
        if (!all && !ci_eq(s->genre, genre)) continue;
        if (q[0] && !(ci_contains(s->title, q) || ci_contains(artist_name(s->artist_id), q) || ci_contains(s->genre, q) || ci_contains(s->mood, q))) continue;
        if (cnt++) sb_add(&res->body, ",");
        song_json(&res->body, s);
    }
    sb_fmt(&res->body, "],\"total\":%d}", cnt);
    free(a);
}
static void h_suggest(Request *rq, ApiRes *res) {
    char q[64] = ""; int ids[8];
    qs_get(rq->query, "q", q, sizeof q);
    int n = q[0] ? song_suggest(q, ids, 8) : 0;
    songs_by_ids_json(&res->body, ids, n, "songs");
}
static void h_upload(Request *rq, ApiRes *res, User *me) {
    char fn[200] = "", genre[40] = "Uploaded", mood[30] = "";
    qs_get(rq->query, "filename", fn, sizeof fn); qs_get(rq->query, "genre", genre, sizeof genre); qs_get(rq->query, "mood", mood, sizeof mood);
    const char *dot = strrchr(fn, '.');
    if (!dot || !(ci_eq(dot, ".mp3") || ci_eq(dot, ".wav") || ci_eq(dot, ".flac"))) ERR(400, "Sirf MP3, WAV ya FLAC allowed hai");
    if (!rq->body_len) ERR(400, "Empty file");
    char ext[8]; str_copy(ext, sizeof ext, dot); str_lower(ext);
    char name[200]; str_copy(name, sizeof name, fn); name[dot - fn] = 0; str_clean(name);
    char title[100], artist[80] = "Unknown";
    char *sep = strstr(name, " - ");
    if (sep) { *sep = 0; str_copy(artist, sizeof artist, name); str_copy(title, sizeof title, sep + 3); }
    else str_copy(title, sizeof title, name);
    char stored[80], hex[17], path[400];
    rand_hex(hex, 8); snprintf(stored, sizeof stored, "%s%s", hex, ext);
    snprintf(path, sizeof path, "%s/%s", store_upload_dir(), stored);
    FILE *f = fopen(path, "wb");
    if (!f) ERR(500, "Server file save nahi kar paya");
    fwrite(rq->body, 1, rq->body_len, f); fclose(f);
    Song *s = song_add(title, artist, genre, mood, 0, stored, me->id, 0);
    store_save();
    res->status = 201;
    sb_add(&res->body, "{\"song\":"); song_json(&res->body, s); sb_add(&res->body, "}");
}
static void h_song_delete(ApiRes *res, User *me, int id) {
    Song *s = song_by_id(id);
    if (!s) ERR(404, "Song not found");
    if (!s->uploader) ERR(403, "Built-in demo songs delete nahi ho sakte");
    if (!me->admin && s->uploader != me->id) ERR(403, "Sirf admin ya uploader delete kar sakta hai");
    song_delete(id); store_save();
    sb_add(&res->body, "{\"ok\":true}");
}

/* ---------- playlists ---------- */
static void h_playlists(Request *rq, ApiRes *res, User *me, char **seg, int ns) {
    const char *js = (const char *)rq->body;
    if (ns == 2 && IS("GET")) {
        sb_add(&res->body, "{\"playlists\":[");
        for (Playlist *p = me->playlists; p; p = p->next) { if (p != me->playlists) sb_add(&res->body, ","); playlist_json(&res->body, p); }
        sb_add(&res->body, "]}"); return;
    }
    if (ns == 2 && IS("POST")) {
        char name[80] = ""; json_str(js, "name", name, sizeof name); str_clean(name);
        if (!name[0]) ERR(400, "Playlist name chahiye");
        Playlist *p = pl_create(me, name); store_save(); res->status = 201; playlist_json(&res->body, p); return;
    }
    if (ns == 3 && IS("POST") && !strcmp(seg[2], "smart")) {
        char mood[40] = ""; json_str(js, "mood", mood, sizeof mood);
        if (!mood[0]) ERR(400, "Mood chahiye");
        char name[80]; snprintf(name, sizeof name, "Smart: %s", mood);
        Playlist *p = pl_create(me, name);
        int n; Song **a = songs_by_title(&n);
        for (int i = 0; i < n; i++) if (ci_eq(a[i]->mood, mood)) pl_add_song(p, a[i]->id);
        free(a); store_save(); res->status = 201; playlist_json(&res->body, p); return;
    }
    if (ns >= 3 && is_num(seg[2])) {
        Playlist *p = pl_find(me, atoi(seg[2]));
        if (!p) ERR(404, "Playlist not found");
        if (ns == 3 && IS("PUT")) {
            char name[80] = ""; json_str(js, "name", name, sizeof name); str_clean(name);
            if (!name[0]) ERR(400, "Name chahiye");
            str_copy(p->name, sizeof p->name, name); store_save(); playlist_json(&res->body, p); return;
        }
        if (ns == 3 && IS("DELETE")) { pl_delete(me, p->id); store_save(); sb_add(&res->body, "{\"ok\":true}"); return; }
        if (ns == 4 && !strcmp(seg[3], "songs") && IS("POST")) {
            int sid; if (!json_int(js, "song_id", &sid) || !song_by_id(sid)) ERR(400, "Valid song_id chahiye");
            int added = pl_add_song(p, sid); store_save();
            sb_fmt(&res->body, "{\"added\":%s}", added ? "true" : "false"); return;
        }
        if (ns == 5 && !strcmp(seg[3], "songs") && IS("DELETE") && is_num(seg[4])) {
            dl_remove_val(&p->songs, atoi(seg[4])); store_save(); sb_add(&res->body, "{\"ok\":true}"); return;
        }
        if (ns == 4 && !strcmp(seg[3], "reorder") && IS("POST")) {
            int from, to;
            if (!json_int(js, "from", &from) || !json_int(js, "to", &to) || !dl_move(&p->songs, from, to)) ERR(400, "Invalid from/to");
            store_save(); playlist_json(&res->body, p); return;
        }
    }
    ERR(404, "Unknown playlist route");
}

/* ---------- queue & history ---------- */
static void h_queue(Request *rq, ApiRes *res, User *me, char **seg, int ns) {
    const char *js = (const char *)rq->body;
    PlayQueue *q = &me->queue;
    if (ns == 2 && IS("GET")) { queue_json(&res->body, me); return; }
    if (ns == 2 && IS("DELETE")) { pq_clear(q); queue_json(&res->body, me); return; }
    if (ns == 3 && IS("POST")) {
        if (!strcmp(seg[2], "set")) {
            int ids[2000], idx = 0, n = json_int_array(js, "ids", ids, 2000);
            json_int(js, "index", &idx);
            if (n < 0) ERR(400, "ids array chahiye");
            int ok[2000], m = 0;
            for (int i = 0; i < n; i++) if (song_by_id(ids[i])) { if (i == idx) idx = m; ok[m++] = ids[i]; }
            pq_set(q, ok, m, idx); queue_json(&res->body, me); return;
        }
        if (!strcmp(seg[2], "add")) {
            int sid; if (!json_int(js, "song_id", &sid) || !song_by_id(sid)) ERR(400, "Valid song_id chahiye");
            pq_enqueue(q, sid); queue_json(&res->body, me); return;
        }
        if (!strcmp(seg[2], "next")) { pq_next(q); queue_json(&res->body, me); return; }
        if (!strcmp(seg[2], "prev")) { pq_prev(q); queue_json(&res->body, me); return; }
        if (!strcmp(seg[2], "remove")) {
            int i; if (!json_int(js, "index", &i) || !pq_remove_at(q, i)) ERR(400, "Invalid index");
            queue_json(&res->body, me); return;
        }
    }
    ERR(404, "Unknown queue route");
}
static void h_history(Request *rq, ApiRes *res, User *me, char **seg, int ns) {
    if (ns == 2 && IS("GET")) {
        int ids[40], n = 0;
        for (SNode *s = me->history.top; s && n < 40; s = s->next) ids[n++] = s->val;
        songs_by_ids_json(&res->body, ids, n, "songs"); return;
    }
    if (ns == 3 && IS("POST") && !strcmp(seg[2], "back")) {          /* stack pop: go back to the previous song */
        int cur, prev;
        if (stack_pop(&me->history, &cur) && stack_peek(&me->history, &prev)) { songs_by_ids_json(&res->body, &prev, 1, "songs"); return; }
        sb_add(&res->body, "{\"songs\":[]}"); return;
    }
    ERR(404, "Unknown history route");
}

/* ---------- admin ---------- */
typedef struct { int users, plays; MaxHeap top; } Stats;
static void stat_song(Song *const *a, int n, Stats *st) {
    for (int i = 0; i < n; i++) { st->plays += a[i]->plays; heap_push(&st->top, a[i]->id, a[i]->plays); }
}
static void h_admin(Request *rq, ApiRes *res, User *me, char **seg, int ns);
static void user_row(User *u, void *cx) {
    SB *b = cx;
    if (b->d[b->len - 1] != '[') sb_add(b, ",");
    user_json(b, u);
}
static void h_admin(Request *rq, ApiRes *res, User *me, char **seg, int ns) {
    if (!me->admin) ERR(403, "Admin access chahiye");
    if (ns == 3 && IS("GET") && !strcmp(seg[2], "stats")) {
        Stats st = { user_count(), 0, {0} }; heap_init(&st.top);
        int n; Song **a = songs_array(&n); stat_song(a, n, &st); free(a);
        sb_fmt(&res->body, "{\"users\":%d,\"songs\":%d,\"playlists\":%d,\"plays\":%d,\"top\":[", st.users, song_count(), playlist_total(), st.plays);
        HeapItem it;
        for (int i = 0; i < 5 && heap_pop(&st.top, &it); i++) {
            Song *s = song_by_id(it.id);
            if (i) sb_add(&res->body, ",");
            song_json(&res->body, s);
        }
        sb_fmt(&res->body, "],\"system\":{\"uptime_s\":%lld,\"requests\":%ld,\"avl_height\":%d,\"graph_edges\":%d}}",
               (now_ms() - g_start_ms) / 1000, g_requests, avl_tree_height(), graph_edges());
        heap_free(&st.top); return;
    }
    if (ns == 3 && IS("GET") && !strcmp(seg[2], "users")) {
        sb_add(&res->body, "{\"users\":[");
        users_each(user_row, &res->body);
        sb_add(&res->body, "]}"); return;
    }
    if (ns == 4 && IS("DELETE") && !strcmp(seg[2], "users") && is_num(seg[3])) {
        int id = atoi(seg[3]); User *u = user_by_id(id);
        if (!u) ERR(404, "User not found");
        if (u->admin || u->id == me->id) ERR(403, "Admin ko remove nahi kar sakte");
        user_delete(id); store_save(); sb_add(&res->body, "{\"ok\":true}"); return;
    }
    ERR(404, "Unknown admin route");
}

/* ---------- dispatcher ---------- */
void api_handle(Request *rq, ApiRes *res) {
    char path[512]; str_copy(path, sizeof path, rq->path);
    char *seg[8]; int ns = split_path(path, seg, 8);
    res->status = 200; sb_init(&res->body);
    if (ns < 2) ERR(404, "Not found");
    const char *r = seg[1];

    if (!strcmp(r, "health") && IS("GET")) {
        sb_fmt(&res->body, "{\"status\":\"ok\",\"uptime_s\":%lld,\"requests\":%ld,\"users\":%d,\"songs\":%d}",
               (now_ms() - g_start_ms) / 1000, g_requests, user_count(), song_count());
        return;
    }
    if (!strcmp(r, "register") && IS("POST")) { h_register(rq, res); return; }
    if (!strcmp(r, "login") && IS("POST")) { h_login(rq, res); return; }

    User *me = auth(rq);
    if (!me) ERR(401, "Login required");

    if (!strcmp(r, "logout") && IS("POST")) { session_end(rq->auth + 7); sb_add(&res->body, "{\"ok\":true}"); return; }
    if (!strcmp(r, "me") && IS("GET")) { sb_add(&res->body, "{\"user\":"); user_json(&res->body, me); sb_add(&res->body, "}"); return; }
    if (!strcmp(r, "suggest") && IS("GET")) { h_suggest(rq, res); return; }
    if (!strcmp(r, "songs")) {
        if (ns == 2 && IS("GET")) { h_songs_list(rq, res); return; }
        if (ns == 3 && IS("POST") && !strcmp(seg[2], "upload")) { h_upload(rq, res, me); return; }
        if (ns == 3 && IS("DELETE") && is_num(seg[2])) { h_song_delete(res, me, atoi(seg[2])); return; }
        if (ns == 4 && is_num(seg[2])) {
            int id = atoi(seg[2]); Song *s = song_by_id(id);
            if (!s) ERR(404, "Song not found");
            if (IS("POST") && !strcmp(seg[3], "play")) {
                s->plays++;
                int top; if (!stack_peek(&me->history, &top) || top != id) stack_push(&me->history, id);
                store_save();
                sb_fmt(&res->body, "{\"plays\":%d}", s->plays); return;
            }
            if (IS("GET") && !strcmp(seg[3], "recommend")) {
                char lim[8] = "6"; qs_get(rq->query, "limit", lim, sizeof lim);
                int k = atoi(lim); if (k < 1) k = 1; if (k > 20) k = 20;
                int ids[20], n = song_recommend(id, ids, k);
                songs_by_ids_json(&res->body, ids, n, "songs"); return;
            }
        }
    }
    if (!strcmp(r, "playlists")) { h_playlists(rq, res, me, seg, ns); return; }
    if (!strcmp(r, "queue")) { h_queue(rq, res, me, seg, ns); return; }
    if (!strcmp(r, "history")) { h_history(rq, res, me, seg, ns); return; }
    if (!strcmp(r, "admin")) { h_admin(rq, res, me, seg, ns); return; }
    ERR(404, "Unknown endpoint");
}
