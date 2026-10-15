#include "graph.h"
#include "heap.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

static int weight(const GNode *a, const GNode *b) {
    int w = 0;
    if (a->artist == b->artist) w += 3;
    if (a->genre[0] && !ci_eq(a->genre, "Uploaded") && ci_eq(a->genre, b->genre)) w += 2;
    if (a->mood[0] && ci_eq(a->mood, b->mood)) w += 1;
    return w;
}
static void add_edge(Adj *a, int to, int w) {
    if (a->n == a->cap) { a->cap = a->cap ? a->cap * 2 : 4; a->e = realloc(a->e, sizeof(Edge) * (size_t)a->cap); }
    a->e[a->n].to = to; a->e[a->n].w = w; a->n++;
}
Graph *graph_build(const GNode *nodes, int n) {
    Graph *g = calloc(1, sizeof *g);
    g->n = n;
    g->nodes = malloc(sizeof(GNode) * (size_t)(n ? n : 1));
    g->adj = calloc((size_t)(n ? n : 1), sizeof(Adj));
    if (n) memcpy(g->nodes, nodes, sizeof(GNode) * (size_t)n);
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++) {
            int w = weight(&g->nodes[i], &g->nodes[j]);
            if (w > 0) { add_edge(&g->adj[i], j, w); add_edge(&g->adj[j], i, w); }
        }
    return g;
}
int graph_edge_count(Graph *g) { int c = 0; for (int i = 0; i < g->n; i++) c += g->adj[i].n; return c / 2; }

int graph_recommend(Graph *g, int song_id, int *out, int k) {
    int src = -1;
    for (int i = 0; i < g->n; i++) if (g->nodes[i].id == song_id) { src = i; break; }
    if (src < 0 || k <= 0) return 0;
    int *dist = malloc(sizeof(int) * (size_t)g->n), *score = calloc((size_t)g->n, sizeof(int)), *q = malloc(sizeof(int) * (size_t)g->n);
    for (int i = 0; i < g->n; i++) dist[i] = -1;
    int qh = 0, qt = 0;
    dist[src] = 0; q[qt++] = src;
    while (qh < qt) {                                      /* BFS, depth <= 2 */
        int u = q[qh++];
        if (dist[u] >= 2) continue;
        for (int i = 0; i < g->adj[u].n; i++) {
            int v = g->adj[u].e[i].to, w = g->adj[u].e[i].w;
            if (v == src) continue;
            score[v] += (dist[u] == 0) ? w * 10 : w;       /* direct neighbours weigh more */
            if (dist[v] < 0) { dist[v] = dist[u] + 1; q[qt++] = v; }
        }
    }
    MaxHeap h; heap_init(&h);
    for (int i = 0; i < g->n; i++) if (score[i] > 0) heap_push(&h, g->nodes[i].id, score[i]);
    int cnt = 0; HeapItem it;
    while (cnt < k && heap_pop(&h, &it)) out[cnt++] = it.id;
    heap_free(&h); free(dist); free(score); free(q);
    return cnt;
}
void graph_free(Graph *g) {
    if (!g) return;
    for (int i = 0; i < g->n; i++) free(g->adj[i].e);
    free(g->adj); free(g->nodes); free(g);
}
