/* graph.h - song similarity graph + BFS recommendations (DSA: Graph, BFS, Heap) */
#ifndef GRAPH_H
#define GRAPH_H
typedef struct { int id; int artist; char genre[40]; char mood[30]; } GNode;
typedef struct { int to, w; } Edge;
typedef struct { Edge *e; int n, cap; } Adj;
typedef struct { int n; GNode *nodes; Adj *adj; } Graph;

Graph *graph_build(const GNode *nodes, int n);           /* adjacency lists, weighted */
int    graph_recommend(Graph *g, int song_id, int *out, int k);   /* ids ranked best-first */
int    graph_edge_count(Graph *g);
void   graph_free(Graph *g);
#endif
