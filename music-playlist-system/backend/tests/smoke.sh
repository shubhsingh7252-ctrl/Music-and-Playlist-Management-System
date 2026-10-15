#!/bin/sh
# End-to-end API test. Usage: sh tests/smoke.sh   (builds nothing; run `make` first)
BIN=${BIN:-./musicms}; PORT=${PORT:-18080}; D=$(mktemp -d); B=http://localhost:$PORT
$BIN -p $PORT -d "$D" -w ../frontend >/dev/null 2>>${LOG:-/dev/null} & PID=$!
trap 'kill $PID 2>/dev/null; rm -rf "$D"' EXIT
sleep 1
ok=0; bad=0
t() { if echo "$2" | grep -q "$3"; then ok=$((ok+1)); else bad=$((bad+1)); echo "FAIL: $1 -> $2" | head -c 300; echo; fi; }
j() { curl -s "$@"; }

t health "$(j $B/api/health)" '"status":"ok"'
t noauth "$(j $B/api/songs)" 'Login required'
R=$(j -X POST $B/api/register -d '{"username":"admin1","email":"a@b.com","password":"pass1"}')
t register "$R" '"role":"admin"'
TA=$(echo "$R" | sed 's/.*"token":"\([a-f0-9]*\)".*/\1/')
R=$(j -X POST $B/api/register -d '{"username":"bob","email":"b@b.com","password":"pass2"}')
t second_user_role "$R" '"role":"user"'
TB=$(echo "$R" | sed 's/.*"token":"\([a-f0-9]*\)".*/\1/')
t dup "$(j -X POST $B/api/register -d '{"username":"BOB","email":"b@b.com","password":"pass2"}')" 'already taken'
t badpw "$(j -X POST $B/api/register -d '{"username":"zed","email":"z@z.com","password":"ab"}')" 'kam se kam 4'
t login_bad "$(j -X POST $B/api/login -d '{"username":"bob","password":"nope"}')" 'Wrong'
t login_ok "$(j -X POST $B/api/login -d '{"username":"bob","password":"pass2"}')" '"token"'
A="Authorization: Bearer $TA"; U="Authorization: Bearer $TB"

S=$(j -H "$A" $B/api/songs)
t songs8 "$S" '"total":8'
t sorted_title "$S" '"title":"Deep Focus".*"title":"Golden Hour"'
t search "$(j -H "$A" "$B/api/songs?q=lofi")" '"total":2'
t genre "$(j -H "$A" "$B/api/songs?genre=Pop")" '"total":2'
t sort_artist "$(j -H "$A" "$B/api/songs?sort=artist" | head -c 120)" 'Aria Lane'
t suggest "$(j -H "$A" "$B/api/suggest?q=ne")" 'Neon Drive'
t recommend "$(j -H "$A" $B/api/songs/1/recommend)" 'Static Hearts'

t pl_default "$(j -H "$U" $B/api/playlists)" 'Favourites'
R=$(j -H "$U" -X POST $B/api/playlists -d '{"name":"Gym"}'); t pl_create "$R" '"name":"Gym"'
PID_=$(echo "$R" | sed 's/.*"id":\([0-9]*\).*/\1/')
for s in 1 2 3; do j -H "$U" -X POST $B/api/playlists/$PID_/songs -d "{\"song_id\":$s}" >/dev/null; done
t pl_dup "$(j -H "$U" -X POST $B/api/playlists/$PID_/songs -d '{"song_id":1}')" '"added":false'
t pl_reorder "$(j -H "$U" -X POST $B/api/playlists/$PID_/reorder -d '{"from":0,"to":2}')" '"songs":\[2,3,1\]'
t pl_remove_song "$(j -H "$U" -X DELETE $B/api/playlists/$PID_/songs/3 && j -H "$U" $B/api/playlists)" '"songs":\[2,1\]'
t pl_smart "$(j -H "$U" -X POST $B/api/playlists/smart -d '{"mood":"Calm"}')" '"songs":\[2,6\]'
t pl_isolated "$(j -H "$A" $B/api/playlists)" '^{"playlists":\[{"id":[0-9]*,"name":"Favourites","songs":\[\]}\]}$'

t q_set "$(j -H "$U" -X POST $B/api/queue/set -d '{"ids":[4,5,6],"index":1}')" '"index":1'
t q_next "$(j -H "$U" -X POST $B/api/queue/next)" '"index":2'
t q_next_end "$(j -H "$U" -X POST $B/api/queue/next)" '"index":2'
t q_prev "$(j -H "$U" -X POST $B/api/queue/prev)" '"index":1'
t q_add "$(j -H "$U" -X POST $B/api/queue/add -d '{"song_id":8}')" '\[4,5,6,8\]'

j -H "$U" -X POST $B/api/songs/5/play >/dev/null; j -H "$U" -X POST $B/api/songs/5/play >/dev/null; j -H "$U" -X POST $B/api/songs/2/play >/dev/null
t history "$(j -H "$U" $B/api/history | head -c 60)" '"id":2'
t hist_back "$(j -H "$U" -X POST $B/api/history/back | head -c 60)" '"id":5'

t admin_forbidden "$(j -H "$U" $B/api/admin/stats)" 'Admin access'
ST=$(j -H "$A" $B/api/admin/stats)
t stats_plays "$ST" '"plays":3'
t stats_top "$ST" '"top":\[{"id":5'
t admin_users "$(j -H "$A" $B/api/admin/users)" 'bob'

# audio streaming + range
H=$(curl -s -D - -o /dev/null -H "Range: bytes=0-99" $B/api/songs/1/stream)
t range206 "$H" '206 Partial'; t range_hdr "$H" 'Content-Range: bytes 0-99/768044'
t wav_magic "$(curl -s -r 0-3 $B/api/songs/3/stream)" 'RIFF'

# upload mp3 (fake bytes), stream back, delete
printf 'ID3FAKEMP3DATA' > "$D/x.mp3"
R=$(j -H "$U" -X POST "$B/api/songs/upload?filename=Daft%20Punk%20-%20One%20More%20Time.mp3" --data-binary @"$D/x.mp3")
t upload "$R" '"title":"One More Time","artist":"Daft Punk"'
SID=$(echo "$R" | sed 's/.*"id":\([0-9]*\),"title".*/\1/')
t upload_stream "$(curl -s $B/api/songs/$SID/stream)" 'FAKEMP3'
t upload_badext "$(j -H "$U" -X POST "$B/api/songs/upload?filename=a.exe" --data-binary @"$D/x.mp3")" 'Sirf MP3'
t suggest_upload "$(j -H "$A" "$B/api/suggest?q=daft")" 'One More Time'
j -H "$U" -X POST $B/api/queue/add -d "{\"song_id\":$SID}" >/dev/null
t demo_nodelete "$(j -H "$A" -X DELETE $B/api/songs/1)" 'demo songs'
t other_nodelete "$(j -H "$(echo "Authorization: Bearer $TA")" -X DELETE $B/api/songs/$SID)" '"ok":true'
t deleted_gone "$(j -H "$U" $B/api/queue)" '^{"items":\[4,5,6,8\]'

# persistence: restart server, data must survive
kill $PID; sleep 1
$BIN -p $PORT -d "$D" -w ../frontend >/dev/null 2>>${LOG:-/dev/null} & PID=$!
sleep 1
t persist_login "$(j -X POST $B/api/login -d '{"username":"bob","password":"pass2"}')" '"token"'
TB=$(j -X POST $B/api/login -d '{"username":"bob","password":"pass2"}' | sed 's/.*"token":"\([a-f0-9]*\)".*/\1/')
t persist_pl "$(j -H "Authorization: Bearer $TB" $B/api/playlists)" '"name":"Gym","songs":\[2,1\]'
t persist_plays "$(j -X POST $B/api/login -d '{"username":"admin1","password":"pass1"}' >/dev/null; j -H "$A" $B/api/health)" '"songs":8'

# remove user
t sessions_reset "$(j -H "$A" $B/api/songs)" 'Login required'
TA=$(j -X POST $B/api/login -d '{"username":"admin1","password":"pass1"}' | sed 's/.*"token":"\([a-f0-9]*\)".*/\1/'); A="Authorization: Bearer $TA"
t del_admin_blocked "$(j -H "$A" -X DELETE $B/api/admin/users/1)" 'Admin ko'
BOBID=$(j -H "$A" $B/api/admin/users | sed 's/.*"id":\([0-9]*\),"username":"bob".*/\1/')
t del_user "$(j -H "$A" -X DELETE $B/api/admin/users/$BOBID)" '"ok":true'
t del_user_gone "$(j -X POST $B/api/login -d '{"username":"bob","password":"pass2"}')" 'Wrong'
# static + traversal
t static "$(curl -s $B/)" 'DOCTYPE'
t traversal "$(curl -s --path-as-is $B/../backend/Makefile)" 'Forbidden\|Not found'
# big header/garbage robustness
printf 'GARBAGE\r\n\r\n' | nc -q1 localhost $PORT >/dev/null 2>&1
t still_alive "$(j $B/api/health)" '"status":"ok"'
echo "passed=$ok failed=$bad"; [ $bad -eq 0 ]
