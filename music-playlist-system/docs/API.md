# REST API (JSON)

Auth: `Authorization: Bearer <token>` (register/login se milta hai, 7 din valid).
Errors: `{"error":"message"}` with HTTP 400/401/403/404/409/413.

| Method | Path | Body / Query | Notes |
|---|---|---|---|
| GET | `/api/health` | — | public; uptime, requests, counts |
| POST | `/api/register` | `{username,email,password}` | pehla user = admin; auto login |
| POST | `/api/login` | `{username,password}` | |
| POST | `/api/logout` | — | |
| GET | `/api/me` | — | |
| GET | `/api/songs` | `?q=&genre=&sort=title\|artist\|plays\|newest` | title sort = AVL in-order |
| GET | `/api/suggest` | `?q=ne` | Trie autocomplete (max 8) |
| GET | `/api/songs/:id/stream` | `Range` header | **public**, audio stream, seek support (206) |
| POST | `/api/songs/:id/play` | — | plays++ and history push |
| GET | `/api/songs/:id/recommend` | `?limit=6` | Graph + BFS + heap |
| POST | `/api/songs/upload` | `?filename=Artist - Title.mp3` + raw file body | MP3/WAV/FLAC, max 60 MB |
| DELETE | `/api/songs/:id` | — | uploader ya admin; demo songs protected |
| GET/POST | `/api/playlists` | `{name}` | |
| PUT/DELETE | `/api/playlists/:id` | `{name}` | |
| POST | `/api/playlists/:id/songs` | `{song_id}` | |
| DELETE | `/api/playlists/:id/songs/:songId` | — | |
| POST | `/api/playlists/:id/reorder` | `{from,to}` | drag & drop |
| POST | `/api/playlists/smart` | `{mood}` | mood se auto playlist |
| GET/DELETE | `/api/queue` | — | queue state `{items:[ids],index}` |
| POST | `/api/queue/set` `/add` `/next` `/prev` `/remove` | `{ids,index}` / `{song_id}` / `{index}` | |
| GET | `/api/history` | — | stack, latest first |
| POST | `/api/history/back` | — | stack pop → previous song |
| GET | `/api/admin/stats` | — | admin only: counts, top-5 (heap), AVL height, graph edges |
| GET | `/api/admin/users` | — | admin only |
| DELETE | `/api/admin/users/:id` | — | admin only (admin remove nahi hota) |
