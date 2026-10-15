/* store.h - in-memory database built from the DSA modules + file persistence */
#ifndef STORE_H
#define STORE_H
#include "dlist.h"
#include "queue.h"
#include "stack.h"
#include <time.h>

typedef struct { int id; char name[80]; } Artist;
typedef struct {
    int id; char title[100]; int artist_id; char genre[40]; char mood[30];
    int seed; char file[80]; int uploader; int plays; int duration; long created;
} Song;
typedef struct Playlist { int id; char name[80]; DList songs; struct Playlist *next; } Playlist;
typedef struct User {
    int id; char username[40]; char email[100]; char salt[17]; char hash[65];
    int admin; long created;
    Playlist *playlists;          /* singly linked list of playlists */
    PlayQueue queue;              /* server-side play queue */
    Stack history;                /* recently played (stack) */
} User;

extern long g_requests;
extern long long g_start_ms;

void store_init(const char *datadir);
void store_lock(void);
void store_unlock(void);
const char *store_upload_dir(void);
void store_save(void);

/* users & sessions */
User *user_register(const char *username, const char *email, const char *password);  /* NULL if taken */
User *user_login(const char *username, const char *password);
User *user_by_id(int id);
int   user_delete(int id);
int   user_count(void);
void  users_each(void (*fn)(User *, void *), void *ctx);
void  session_new(User *u, char token[65]);
User *session_user(const char *token);
void  session_end(const char *token);

/* songs */
Song *song_by_id(int id);
const char *artist_name(int artist_id);
Song *song_add(const char *title, const char *artist, const char *genre, const char *mood,
               int seed, const char *file, int uploader, int duration);
int   song_delete(int id);
int   song_count(void);
Song **songs_array(int *n);              /* hash order; caller frees */
Song **songs_by_title(int *n);           /* AVL in-order; caller frees */
int   song_suggest(const char *prefix, int *out, int max);   /* trie */
int   song_recommend(int id, int *out, int k);               /* graph + BFS + heap */
int   graph_edges(void);
int   avl_tree_height(void);

/* playlists */
Playlist *pl_create(User *u, const char *name);
Playlist *pl_find(User *u, int id);
int       pl_delete(User *u, int id);
int       pl_add_song(Playlist *p, int song_id);
int       playlist_total(void);
#endif
