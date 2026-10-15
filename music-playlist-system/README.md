# ♫ Music & Playlist Management System (BCA / DSA Project)

PPT ke hisaab se **three-tier** project: Browser (frontend) ↔ **C backend** (REST API + custom HTTP server) ↔ file database.
Backend poora **C** mein hai, aur DSA ke concepts — Hash Table, Linked List, Queue, Stack, AVL Tree, Trie, Heap, Graph+BFS, Merge Sort — real features ke peeche use hue hain.

## Chalane ka tareeka

**Windows** (MinGW-w64 gcc chahiye — https://winlibs.com, PATH mein add karo)
```
build.bat      (ek baar)
run.bat        (browser khud khul jata hai: http://localhost:8080)
```
**Linux / macOS**
```
./run.sh            # ya ./run.sh 9000 (alag port)
```
Pehla register hone wala user **Admin** banta hai.

> ⚠️ Mere sandbox mein ye **Linux** par build + test hua hai. Windows build script standard MinGW ke liye likha hai par maine usko yahan chala ke nahi dekha — agar koi compile error aaye to error message bhej dena.

## Features (PPT modules ke saath)
| PPT Module | Kya mila |
|---|---|
| User | Register/Login (salted hash), token session, profile + theme |
| Library | 8 demo songs + MP3/WAV/FLAC upload ("Artist - Title.mp3"), genre chips, sort |
| Playlist | Create/rename/delete, drag-and-drop reorder, **Smart playlist** (mood se) |
| Search | Keyword search, real-time suggestions (**Trie**), voice search 🎤 |
| Playback | Play/pause/next/prev/seek (HTTP Range), volume, **server-side queue**, EQ bass boost |
| Admin | Stats, top songs, system health (uptime, requests, AVL height), user remove, song moderation |
| Future scope (bonus) | ✨ Similar songs (graph recommendation), 🕘 History (stack) |

## Folder structure
```
music-playlist-system/
├── backend/
│   ├── src/          C source (main, http, routes, store + 10 DSA modules)
│   ├── tests/        test_ds.c (228 unit checks), smoke.sh (53 API checks)
│   └── Makefile
├── frontend/         index.html, css/, js/  (C server isi folder ko serve karta hai)
├── docs/             DSA_MAPPING.md, API.md, VIVA_QA.md, schema.sql
├── data/             (run karne par banta hai) musicms.db + uploads/
├── build.bat  run.bat  run.sh
└── README.md
```

## Tests
```
cd backend
make test            # DSA unit tests
make && sh tests/smoke.sh    # end-to-end API test (server khud start/stop hota hai)
```

## Architecture (slide 5)
- **Presentation**: `frontend/` — Vanilla JS, REST calls.
- **Application**: `backend/src/routes.c` (API), `store.c` (business logic), `http.c` (server). Thread per connection + ek global mutex.
- **Data**: in-memory DSA structures, `data/musicms.db` mein atomic save; uploads `data/uploads/` mein.

## Honest limitations
- DB MySQL nahi, text file hai (schema `docs/schema.sql` mein MySQL version ready hai).
- Login sessions memory mein hain: server restart ke baad dobara login karna padega.
- HTTP only (HTTPS nahi); password hashing salted + 3000-round SHA-256 hai (production mein bcrypt/Argon2).
- Search ek linear scan hai; sirf suggestions Trie se aate hain.
- PPT mein Node/React likha tha; DSA-in-C requirement ke liye backend C mein hai aur frontend vanilla JS.

## Deploy
Ye ek C binary + static files hai. Kisi bhi Linux VPS par `make -C backend && PORT=8080 ./backend/musicms -w frontend -d data`. (Netlify/GitHub Pages sirf frontend host karte hain — backend wahan nahi chalega.)

Viva ke liye `docs/VIVA_QA.md` padh lena.
