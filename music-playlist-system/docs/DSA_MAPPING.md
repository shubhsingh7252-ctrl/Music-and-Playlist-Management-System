# DSA Mapping — kaunsa data structure kahan use hua

| # | Data Structure / Algorithm | File | Project mein kaam | Complexity |
|---|---|---|---|---|
| 1 | **Hash Table** (separate chaining, auto-rehash, load factor 0.75) | `hashtable.c` | users by id/username, songs by id, artists, login sessions (token → user) | get/put/remove avg **O(1)** |
| 2 | **Doubly Linked List** | `dlist.c` | Playlist ke songs ka order; drag-and-drop reorder (`dl_move`), add/remove | insert-end O(1), remove O(1) (node mila ho to), move O(n) |
| 3 | **Singly Linked List** | `store.c` (`Playlist *next`) | Har user ki playlists ki list | O(n) |
| 4 | **Queue** (linked list + cursor) | `queue.c` | Play queue: enqueue (add to queue), next/prev, remove | enqueue O(1), next/prev O(1) |
| 5 | **Stack** (bounded, LIFO) | `stack.c` | "Recently played" history; ⏮ pe `pop` se pichla gaana | push/pop O(1) |
| 6 | **AVL Tree** (self-balancing BST) | `avl.c` | Library hamesha title ke hisaab se sorted (in-order traversal) | insert/delete/find **O(log n)** |
| 7 | **Trie** (prefix tree) | `trie.c` | Search-box autocomplete suggestions (title/artist/genre words) | O(L) prefix length |
| 8 | **Binary Max-Heap / Priority Queue** | `heap.c` | Admin "Top 5 songs"; recommendations ka ranking | push/pop O(log n) |
| 9 | **Merge Sort** (stable) | `sort.c` | Library sort: artist / most played / newest | **O(n log n)** |
| 10 | **Graph** (adjacency list, weighted) + **BFS** | `graph.c` | Similar songs ✨: edge weight = same artist(3) + same genre(2) + same mood(1); BFS depth 2 + heap se top-k | build O(n²), BFS O(V+E) |
| 11 | **Hashing (SHA-256)** | `sha256.c`, `store.c` | Password: salt + 3000 rounds SHA-256 (plain password kabhi save nahi) | — |
| 12 | **String algos** | `util.c`, `json.c` | Case-insensitive substring search, URL decode, JSON parse | O(n·m) search |

## Flow example: "Neon Drive" ke similar songs
`GET /api/songs/1/recommend` → `song_recommend()` → graph (lazily rebuilt jab songs badlein) → **BFS** se depth ≤ 2 neighbours → score = weight×10 (direct) ya weight (2-hop) → **max-heap** se top-k → JSON.

## Tests
`make test` (backend folder) — har data structure ke liye 228 assertions (balance of AVL after 1000 inserts + 500 deletes, heap order, trie prefix, SHA-256 known vectors, etc.).
