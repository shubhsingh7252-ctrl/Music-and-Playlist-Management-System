#include "store.h"
#include "hashtable.h"
#include "avl.h"
#include "trie.h"
#include "graph.h"
#include "sha256.h"
#include "platform.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct { int user_id; time_t expires; } Session;

static mutex_t g_mu;
static HashTable *users_id, *users_name, *songs_id, *artists_id, *artists_name, *sessions;
static AvlNode *title_tree;
static TrieNode *trie;
static Graph *graph; static int graph_dirty = 1;
static int next_user = 1, next_song = 1, next_artist = 1, next_pl = 1;
static char db_path[300], upload_dir[300];
long g_requests = 0;
long long g_start_ms = 0;

void store_lock(void) { mutex_lock_(&g_mu); }
void store_unlock(void) { mutex_unlock_(&g_mu); }
const char *store_upload_dir(void) { return upload_dir; }
static void ikey(char *b, int id) { snprintf(b, 24, "%d", id); }

/* ---------- artists ---------- */
static Artist *artist_register(Artist *a) {
    char k[24], low[80];
    ikey(k, a->id); ht_put(artists_id, k, a);
    str_copy(low, sizeof low, a->name); str_lower(low); ht_put(artists_name, low, a);
    if (a->id >= next_artist) next_artist = a->id + 1;
    return a;
}
static Artist *artist_get_or_create(const char *name) {
    char low[80]; str_copy(low, sizeof low, name); str_lower(low);
    Artist *a = ht_get(artists_name, low);
    if (a) return a;
    a = calloc(1, sizeof *a); a->id = next_artist; str_copy(a->name, sizeof a->name, name);
    return artist_register(a);
}
const char *artist_name(int id) {
    char k[24]; ikey(k, id);
    Artist *a = ht_get(artists_id, k);
    return a ? a->name : "Unknown";
}

/* ---------- song indexes (AVL + trie + graph) ---------- */
static void title_key(char *out, const Song *s) {
    char low[100]; str_copy(low, sizeof low, s->title); str_lower(low);
    snprintf(out, 120, "%s\x01%08d", low, s->id);
}
static void index_text(const char *text, int id, int add) {
    char w[64]; int n = 0;
    for (const char *p = text;; p++) {
        if (*p && isalnum((unsigned char)*p) && n < 63) { w[n++] = (char)tolower((unsigned char)*p); continue; }
        if (n) { w[n] = 0; if (add) trie_add(trie, w, id); else trie_del(trie, w, id); n = 0; }
        if (!*p) break;
    }
}
static void index_song(Song *s, int add) {
    char key[120]; title_key(key, s);
    if (add) title_tree = avl_insert(title_tree, key, s); else title_tree = avl_remove(title_tree, key);
    index_text(s->title, s->id, add); index_text(artist_name(s->artist_id), s->id, add); index_text(s->genre, s->id, add);
    graph_dirty = 1;
}
static Song *song_register(Song *s) {
    char k[24]; ikey(k, s->id); ht_put(songs_id, k, s);
    if (s->id >= next_song) next_song = s->id + 1;
    index_song(s, 1);
    return s;
}

/* ---------- passwords ---------- */
static void hash_password(const char *salt, const char *pw, char out[65]) {
    char buf[400]; snprintf(buf, sizeof buf, "%s:%s", salt, pw);
    sha256_hex(buf, strlen(buf), out);
    for (int i = 0; i < 3000; i++) {                       /* key stretching */
        snprintf(buf, sizeof buf, "%s%s", out, salt);
        sha256_hex(buf, strlen(buf), out);
    }
}
static int const_eq(const char *a, const char *b) {
    unsigned char d = 0; size_t la = strlen(a), lb = strlen(b);
    if (la != lb) return 0;
    for (size_t i = 0; i < la; i++) d |= (unsigned char)(a[i] ^ b[i]);
    return d == 0;
}

/* ---------- users ---------- */
static void user_register_ptr(User *u) {
    char k[24], low[40]; ikey(k, u->id); ht_put(users_id, k, u);
    str_copy(low, sizeof low, u->username); str_lower(low); ht_put(users_name, low, u);
    if (u->id >= next_user) next_user = u->id + 1;
}
static User *user_alloc(void) {
    User *u = calloc(1, sizeof *u);
    pq_init(&u->queue); stack_init(&u->history, 30);
    return u;
}
User *user_register(const char *username, const char *email, const char *password) {
    char low[40]; str_copy(low, sizeof low, username); str_lower(low);
    if (ht_get(users_name, low)) return NULL;
    User *u = user_alloc();
    u->id = next_user; str_copy(u->username, sizeof u->username, username); str_copy(u->email, sizeof u->email, email);
    rand_hex(u->salt, 8);
    hash_password(u->salt, password, u->hash);
    u->admin = (user_count() == 0);                        /* first user becomes admin */
    u->created = (long)time(NULL);
    user_register_ptr(u);
    pl_create(u, "Favourites");
    return u;
}
User *user_login(const char *username, const char *password) {
    char low[40], h[65]; str_copy(low, sizeof low, username); str_lower(low);
    User *u = ht_get(users_name, low);
    if (!u) { hash_password("0000000000000000", password, h); return NULL; }   /* equalise timing */
    hash_password(u->salt, password, h);
    return const_eq(h, u->hash) ? u : NULL;
}
User *user_by_id(int id) { char k[24]; ikey(k, id); return ht_get(users_id, k); }
int user_count(void) { return (int)users_id->count; }
typedef struct { void (*fn)(User *, void *); void *ctx; } UserEach;
static void users_visit(const char *k, void *v, void *cx) { (void)k; UserEach *c = cx; c->fn(v, c->ctx); }
void users_each(void (*fn)(User *, void *), void *ctx) {
    UserEach c = { fn, ctx };
    ht_foreach(users_id, users_visit, &c);
}
static void free_playlists(User *u) {
    Playlist *p = u->playlists;
    while (p) { Playlist *nx = p->next; dl_clear(&p->songs); free(p); p = nx; }
    u->playlists = NULL;
}
static void collect_tokens(const char *k, void *v, void *cx) {
    typedef struct { char (*t)[65]; int n, cap, uid; } T;
    T *t = cx; Session *s = v;
    if (s->user_id != t->uid) return;
    if (t->n == t->cap) { t->cap = t->cap ? t->cap * 2 : 8; t->t = realloc(t->t, (size_t)t->cap * 65); }
    str_copy(t->t[t->n++], 65, k);
}
int user_delete(int id) {
    User *u = user_by_id(id);
    if (!u) return 0;
    struct { char (*t)[65]; int n, cap, uid; } t = { NULL, 0, 0, id };
    ht_foreach(sessions, collect_tokens, &t);
    for (int i = 0; i < t.n; i++) free(ht_remove(sessions, t.t[i]));
    free(t.t);
    char k[24], low[40]; ikey(k, id); ht_remove(users_id, k);
    str_copy(low, sizeof low, u->username); str_lower(low); ht_remove(users_name, low);
    free_playlists(u); pq_clear(&u->queue); stack_clear(&u->history);
    free(u);
    return 1;
}

/* ---------- sessions ---------- */
void session_new(User *u, char token[65]) {
    rand_hex(token, 24);
    Session *s = malloc(sizeof *s); s->user_id = u->id; s->expires = time(NULL) + 7 * 24 * 3600;
    ht_put(sessions, token, s);
}
User *session_user(const char *token) {
    Session *s = ht_get(sessions, token);
    if (!s) return NULL;
    User *u = user_by_id(s->user_id);
    if (!u || s->expires < time(NULL)) { free(ht_remove(sessions, token)); return NULL; }
    return u;
}
void session_end(const char *token) { free(ht_remove(sessions, token)); }

/* ---------- songs ---------- */
Song *song_by_id(int id) { char k[24]; ikey(k, id); return ht_get(songs_id, k); }
int song_count(void) { return (int)songs_id->count; }
Song *song_add(const char *title, const char *artist, const char *genre, const char *mood,
               int seed, const char *file, int uploader, int duration) {
    Song *s = calloc(1, sizeof *s);
    s->id = next_song;
    str_copy(s->title, sizeof s->title, title); str_clean(s->title);
    if (!s->title[0]) str_copy(s->title, sizeof s->title, "Untitled");
    char an[80]; str_copy(an, sizeof an, artist && *artist ? artist : "Unknown"); str_clean(an);
    if (!an[0]) str_copy(an, sizeof an, "Unknown");
    s->artist_id = artist_get_or_create(an)->id;
    str_copy(s->genre, sizeof s->genre, genre ? genre : ""); str_clean(s->genre);
    str_copy(s->mood, sizeof s->mood, mood ? mood : ""); str_clean(s->mood);
    s->seed = seed; str_copy(s->file, sizeof s->file, file ? file : "");
    s->uploader = uploader; s->duration = duration; s->created = (long)time(NULL);
    return song_register(s);
}
static void drop_refs(User *u, void *ctx) {
    int id = *(int *)ctx;
    for (Playlist *p = u->playlists; p; p = p->next) dl_remove_val(&p->songs, id);
    pq_remove_val(&u->queue, id); stack_remove_val(&u->history, id);
}
int song_delete(int id) {
    Song *s = song_by_id(id);
    if (!s) return 0;
    index_song(s, 0);
    char k[24]; ikey(k, id); ht_remove(songs_id, k);
    users_each(drop_refs, &id);
    if (s->file[0]) { char path[400]; snprintf(path, sizeof path, "%s/%s", upload_dir, s->file); remove(path); }
    free(s);
    return 1;
}
static void gather(const char *k, void *v, void *cx) { (void)k; typedef struct { Song **a; int n; } G; G *g = cx; g->a[g->n++] = v; }
Song **songs_array(int *n) {
    struct { Song **a; int n; } g = { malloc(sizeof(Song *) * (size_t)(song_count() + 1)), 0 };
    ht_foreach(songs_id, gather, &g);
    *n = g.n; return g.a;
}
static void avl_gather(void *v, void *cx) { struct { Song **a; int n; } *g = cx; g->a[g->n++] = v; }
Song **songs_by_title(int *n) {
    struct { Song **a; int n; } g = { malloc(sizeof(Song *) * (size_t)(song_count() + 1)), 0 };
    avl_inorder(title_tree, avl_gather, &g);
    *n = g.n; return g.a;
}
int song_suggest(const char *prefix, int *out, int max) {
    const char *p = prefix, *last = prefix;                /* use the last word typed */
    for (; *p; p++) if (!isalnum((unsigned char)*p)) last = p + 1;
    return trie_prefix(trie, last, out, max);
}
static void ensure_graph(void) {
    if (!graph_dirty && graph) return;
    graph_free(graph);
    int n; Song **a = songs_array(&n);
    GNode *nodes = malloc(sizeof(GNode) * (size_t)(n ? n : 1));
    for (int i = 0; i < n; i++) {
        nodes[i].id = a[i]->id; nodes[i].artist = a[i]->artist_id;
        str_copy(nodes[i].genre, sizeof nodes[i].genre, a[i]->genre); str_copy(nodes[i].mood, sizeof nodes[i].mood, a[i]->mood);
    }
    graph = graph_build(nodes, n); graph_dirty = 0;
    free(nodes); free(a);
}
int song_recommend(int id, int *out, int k) { ensure_graph(); return graph_recommend(graph, id, out, k); }
int graph_edges(void) { ensure_graph(); return graph_edge_count(graph); }
int avl_tree_height(void) { return avl_height(title_tree); }

/* ---------- playlists ---------- */
Playlist *pl_create(User *u, const char *name) {
    Playlist *p = calloc(1, sizeof *p);
    p->id = next_pl++; str_copy(p->name, sizeof p->name, name); str_clean(p->name);
    if (!p->name[0]) str_copy(p->name, sizeof p->name, "Untitled");
    dl_init(&p->songs);
    Playlist **pp = &u->playlists;
    while (*pp) pp = &(*pp)->next;
    *pp = p;
    return p;
}
Playlist *pl_find(User *u, int id) { for (Playlist *p = u->playlists; p; p = p->next) if (p->id == id) return p; return NULL; }
int pl_delete(User *u, int id) {
    Playlist **pp = &u->playlists;
    while (*pp) {
        if ((*pp)->id == id) { Playlist *p = *pp; *pp = p->next; dl_clear(&p->songs); free(p); return 1; }
        pp = &(*pp)->next;
    }
    return 0;
}
int pl_add_song(Playlist *p, int song_id) {
    if (dl_find(&p->songs, song_id)) return 0;
    dl_push_back(&p->songs, song_id, (long)time(NULL));
    return 1;
}
static void count_pl(User *u, void *cx) { for (Playlist *p = u->playlists; p; p = p->next) (*(int *)cx)++; }
int playlist_total(void) { int c = 0; users_each(count_pl, &c); return c; }

/* ---------- persistence (tab separated text file) ---------- */
static void save_user(User *u, void *cx) {
    FILE *f = cx;
    fprintf(f, "U\t%d\t%s\t%s\t%s\t%s\t%d\t%ld\n", u->id, u->username, u->email, u->salt, u->hash, u->admin, u->created);
}
static void save_pl(User *u, void *cx) {
    FILE *f = cx;
    for (Playlist *p = u->playlists; p; p = p->next) {
        fprintf(f, "P\t%d\t%d\t%s\n", p->id, u->id, p->name);
        for (DNode *n = p->songs.head; n; n = n->next) fprintf(f, "X\t%d\t%d\t%ld\n", p->id, n->val, n->added);
    }
}
static void save_artist(const char *k, void *v, void *cx) { (void)k; Artist *a = v; fprintf((FILE *)cx, "A\t%d\t%s\n", a->id, a->name); }
static void save_song(const char *k, void *v, void *cx) {
    (void)k; Song *s = v;
    fprintf((FILE *)cx, "S\t%d\t%s\t%d\t%s\t%s\t%d\t%s\t%d\t%d\t%d\t%ld\n", s->id, s->title, s->artist_id, s->genre, s->mood,
            s->seed, s->file[0] ? s->file : "-", s->uploader, s->plays, s->duration, s->created);
}
void store_save(void) {
    char tmp[320]; snprintf(tmp, sizeof tmp, "%s.tmp", db_path);
    FILE *f = fopen(tmp, "wb");
    if (!f) { fprintf(stderr, "cannot write %s\n", tmp); return; }
    fprintf(f, "#MUSICMS 1\n");
    ht_foreach(artists_id, save_artist, f);
    users_each(save_user, f);
    ht_foreach(songs_id, save_song, f);
    users_each(save_pl, f);
    fclose(f);
    file_replace(tmp, db_path);
}
static int split_tabs(char *line, char **f, int max) {
    int n = 0; f[n++] = line;
    for (char *p = line; *p && n < max; p++) if (*p == '\t') { *p = 0; f[n++] = p + 1; }
    return n;
}
typedef struct { int pid; int sid; long at; } PlItem;
static void load_pl_item(User *u, void *cx) {
    PlItem *x = cx; Playlist *p = pl_find(u, x->pid);
    if (p && song_by_id(x->sid) && !dl_find(&p->songs, x->sid)) dl_push_back(&p->songs, x->sid, x->at);
}
static void load(void) {
    FILE *f = fopen(db_path, "rb");
    if (!f) return;
    char line[2048];
    while (fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = 0;
        char *c[16]; int n = split_tabs(line, c, 16);
        if (c[0][0] == 'A' && n >= 3) {
            Artist *a = calloc(1, sizeof *a); a->id = atoi(c[1]); str_copy(a->name, sizeof a->name, c[2]); artist_register(a);
        } else if (c[0][0] == 'U' && n >= 8) {
            User *u = user_alloc(); u->id = atoi(c[1]);
            str_copy(u->username, sizeof u->username, c[2]); str_copy(u->email, sizeof u->email, c[3]);
            str_copy(u->salt, sizeof u->salt, c[4]); str_copy(u->hash, sizeof u->hash, c[5]);
            u->admin = atoi(c[6]); u->created = atol(c[7]); user_register_ptr(u);
        } else if (c[0][0] == 'S' && n >= 12) {
            Song *s = calloc(1, sizeof *s); s->id = atoi(c[1]);
            str_copy(s->title, sizeof s->title, c[2]); s->artist_id = atoi(c[3]);
            str_copy(s->genre, sizeof s->genre, c[4]); str_copy(s->mood, sizeof s->mood, c[5]);
            s->seed = atoi(c[6]); if (strcmp(c[7], "-")) str_copy(s->file, sizeof s->file, c[7]);
            s->uploader = atoi(c[8]); s->plays = atoi(c[9]); s->duration = atoi(c[10]); s->created = atol(c[11]);
            song_register(s);
        } else if (c[0][0] == 'P' && n >= 4) {
            User *u = user_by_id(atoi(c[2]));
            if (u) { Playlist *p = pl_create(u, c[3]); p->id = atoi(c[1]); if (p->id >= next_pl) next_pl = p->id + 1; }
        } else if (c[0][0] == 'X' && n >= 4) {
            PlItem q = { atoi(c[1]), atoi(c[2]), atol(c[3]) };
            users_each(load_pl_item, &q);
        }
    }
    fclose(f);
}

static void seed_demo(void) {
    static const char *D[8][4] = {
        {"Neon Drive","Pixel Pilot","Synthwave","Energetic"}, {"Midnight Rain","Aria Lane","Lofi","Calm"},
        {"Golden Hour","The Sunsets","Pop","Happy"}, {"Deep Focus","Nova","Ambient","Focus"},
        {"Run The City","Kilo","Hip-Hop","Energetic"}, {"Slow Tides","Aria Lane","Lofi","Calm"},
        {"Static Hearts","Pixel Pilot","Synthwave","Sad"}, {"Monsoon Dance","The Sunsets","Pop","Happy"}};
    for (int i = 0; i < 8; i++) song_add(D[i][0], D[i][1], D[i][2], D[i][3], i + 1, "", 0, 24);
}

void store_init(const char *datadir) {
    mutex_init_(&g_mu);
    g_start_ms = now_ms();
    make_dir(datadir);
    snprintf(db_path, sizeof db_path, "%s/musicms.db", datadir);
    snprintf(upload_dir, sizeof upload_dir, "%s/uploads", datadir);
    make_dir(upload_dir);
    users_id = ht_new(64); users_name = ht_new(64); songs_id = ht_new(128);
    artists_id = ht_new(64); artists_name = ht_new(64); sessions = ht_new(64);
    trie = trie_new();
    load();
    if (song_count() == 0) { seed_demo(); store_save(); }
}
