'use strict';
/* Music & Playlist Management System - frontend. All data comes from the C backend (REST API). */
const BASE = location.protocol === 'file:' ? 'http://localhost:8080' : '';
const $ = s => document.querySelector(s);
const esc = s => String(s ?? '').replace(/[&<>"']/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
const fmt = t => isFinite(t) ? Math.floor(t / 60) + ':' + String(Math.floor(t % 60)).padStart(2, '0') : '0:00';

let token = localStorage.getItem('mp_token'), user = null;
let all = [], byId = new Map(), pls = [], Q = { items: [], index: -1 };
let ctx = [], view = 'lib', plView = null, filter = 'All', query = '', sortBy = 'title', reg = false;
const au = $('#au'); au.crossOrigin = 'anonymous'; au.volume = .8;

/* ---------- API ---------- */
async function api(path, opt = {}) {
  const h = {}; if (token) h.Authorization = 'Bearer ' + token;
  let body; if (opt.body !== undefined) { h['Content-Type'] = 'application/json'; body = JSON.stringify(opt.body); }
  let r;
  try { r = await fetch(BASE + '/api' + path, { method: opt.method || 'GET', headers: h, body }); }
  catch (e) { throw new Error('Server se connect nahi ho pa raha. Backend chalu hai?'); }
  const d = await r.json().catch(() => ({}));
  if (r.status === 401 && token && !opt.anon) { resetSession(); throw new Error(d.error || 'Session expire ho gaya, dobara login karo'); }
  if (!r.ok) throw new Error(d.error || ('Error ' + r.status));
  return d;
}
let tt;
function toast(msg, ok) {
  const t = $('#toast'); t.textContent = msg; t.className = ok ? 'ok' : ''; t.hidden = false;
  clearTimeout(tt); tt = setTimeout(() => t.hidden = true, 3200);
}
const safe = fn => async (...a) => { try { await fn(...a); } catch (e) { toast(e.message); } };

/* ---------- AUTH ---------- */
function resetSession() {
  token = null; user = null; localStorage.removeItem('mp_token'); au.pause();
  $('#app').hidden = true; $('#auth').hidden = false;
}
$('#sw').onclick = e => {
  e.preventDefault(); reg = !reg;
  $('#e').style.display = reg ? '' : 'none'; $('#go').textContent = reg ? 'Register' : 'Login';
  $('#sw').textContent = reg ? 'Have an account? Login' : 'New here? Register';
  $('#am').textContent = reg ? 'Create your account' : 'Login to your library'; $('#err').textContent = '';
};
$('#go').onclick = async () => {
  $('#err').textContent = '';
  try {
    const d = await api(reg ? '/register' : '/login', { method: 'POST', anon: true,
      body: { username: $('#u').value.trim(), email: $('#e').value.trim(), password: $('#pw').value } });
    token = d.token; user = d.user; localStorage.setItem('mp_token', token); $('#pw').value = '';
    await boot();
  } catch (e) { $('#err').textContent = e.message; }
};
['#u', '#e', '#pw'].forEach(s => $(s).addEventListener('keydown', e => { if (e.key === 'Enter') $('#go').click(); }));
$('#lo').onclick = async () => { try { await api('/logout', { method: 'POST' }); } catch (e) {} resetSession(); location.reload(); };

async function loadSongs() { all = (await api('/songs')).songs; byId = new Map(all.map(s => [s.id, s])); }
async function loadPls() { pls = (await api('/playlists')).playlists; }
async function loadQueue() { Q = await api('/queue'); }
async function boot() {
  $('#auth').hidden = true; $('#app').hidden = false;
  $('#admb').style.display = user.role === 'admin' ? '' : 'none';
  await Promise.all([loadSongs(), loadPls(), loadQueue()]);
  go('lib');
}

/* ---------- NAV ---------- */
document.querySelectorAll('nav [data-v]').forEach(b => b.onclick = () => go(b.dataset.v));
function go(v) {
  view = v; plView = null;
  document.querySelectorAll('nav [data-v]').forEach(b => b.classList.toggle('on', b.dataset.v === v));
  render();
}
const VIEWS = () => ({ lib: renderLib, pls: renderPls, q: renderQ, hist: renderHist, adm: renderAdm, set: renderSet });
const render = safe(async () => { await VIEWS()[view](); markNow(); });

/* ---------- ROWS ---------- */
function row(s, o = {}) {
  const now = Q.items[Q.index] === s.id;
  return `<div class="row ${now ? 'now' : ''}" ${o.drag ? `draggable="true" data-i="${o.i}"` : ''}>
<button class="ghost" data-act="play" data-id="${s.id}">${now && !au.paused ? '❚❚' : '▶'}</button>
<div><div class="t">${esc(s.title)}</div><div class="tag">${esc(s.artist)}</div></div>
<span class="tag g">${esc(s.genre)}</span><span class="tag m">${esc(s.mood || '—')}</span>
<span class="acts"><button class="ghost" title="Add to playlist" data-act="add" data-id="${s.id}">＋</button>
<button class="ghost" title="Add to queue" data-act="enq" data-id="${s.id}">⏭</button>
<button class="ghost" title="Similar songs (graph BFS)" data-act="sim" data-id="${s.id}">✨</button>${o.rm ? `<button class="ghost" data-act="rm" data-id="${s.id}">✕</button>` : ''}</span></div>`;
}
function markNow() {
  document.querySelectorAll('#main .row').forEach(r => {
    const b = r.querySelector('[data-act=play]'); if (!b) return;
    const now = Q.items[Q.index] === +b.dataset.id;
    r.classList.toggle('now', now); b.textContent = now && !au.paused ? '❚❚' : '▶';
  });
}

/* ---------- LIBRARY ---------- */
async function renderLib() {
  const genres = ['All', ...new Set(all.map(s => s.genre).filter(Boolean))];
  $('#main').innerHTML = `<h1>Library</h1><div class="top"><input id="sq" list="sg" autocomplete="off" placeholder="Search title, artist, genre, mood…" value="${esc(query)}"><datalist id="sg"></datalist>
<button class="ghost" id="vs" title="Voice search">🎤</button>
<select id="so"><option value="title">A–Z</option><option value="artist">Artist</option><option value="plays">Most played</option><option value="newest">Newest</option></select>
<button class="btn" id="up">Upload</button><input type="file" id="fi" accept="audio/mpeg,audio/wav,audio/flac,.mp3,.wav,.flac" multiple hidden></div>
<div class="chips">${genres.map(g => `<button class="chip ${g === filter ? 'on' : ''}" data-act="genre" data-g="${esc(g)}">${esc(g)}</button>`).join('')}</div>
<p class="tag" id="cnt"></p><div id="lst"></div>`;
  $('#so').value = sortBy;
  let t; const sq = $('#sq');
  sq.oninput = () => { query = sq.value; clearTimeout(t); t = setTimeout(safe(async () => { await refreshList(); await suggest(); }), 220); };
  $('#so').onchange = e => { sortBy = e.target.value; safe(refreshList)(); };
  $('#up').onclick = () => $('#fi').click();
  $('#fi').onchange = safe(async e => {
    for (const f of e.target.files) {
      if (!/\.(mp3|wav|flac)$/i.test(f.name)) { toast(f.name + ': sirf MP3/WAV/FLAC allowed'); continue; }
      const r = await fetch(BASE + '/api/songs/upload?filename=' + encodeURIComponent(f.name), { method: 'POST', headers: { Authorization: 'Bearer ' + token }, body: f });
      const d = await r.json().catch(() => ({}));
      if (!r.ok) throw new Error(d.error || 'Upload fail');
      toast('Uploaded: ' + d.song.title, true);
    }
    await loadSongs(); await renderLib();
  });
  $('#vs').onclick = () => {
    const R = window.SpeechRecognition || window.webkitSpeechRecognition;
    if (!R) return toast('Voice search is browser mein supported nahi');
    const r = new R(); r.onresult = e => { query = e.results[0][0].transcript; sq.value = query; safe(refreshList)(); }; r.start();
  };
  await refreshList();
}
async function refreshList() {
  const p = new URLSearchParams(); if (query) p.set('q', query); if (filter !== 'All') p.set('genre', filter); p.set('sort', sortBy);
  const d = await api('/songs?' + p);
  if (view !== 'lib' || !$('#lst')) return;
  d.songs.forEach(s => byId.set(s.id, s)); ctx = d.songs.map(s => s.id);
  $('#cnt').textContent = `${d.total} tracks · MP3 / WAV / FLAC upload supported ("Artist - Title.mp3" naam se metadata nikalta hai)`;
  $('#lst').innerHTML = d.songs.map(s => row(s)).join('') || '<p class="mu">No results</p>';
}
async function suggest() {
  if (!query.trim()) { $('#sg').innerHTML = ''; return; }
  const d = await api('/suggest?q=' + encodeURIComponent(query));
  if ($('#sg')) $('#sg').innerHTML = d.songs.map(s => `<option value="${esc(s.title)}">`).join('');
}

/* ---------- PLAYLISTS ---------- */
async function renderPls() {
  await loadPls();
  const m = $('#main');
  if (plView) {
    const p = pls.find(x => x.id === plView);
    if (!p) { plView = null; return renderPls(); }
    const songs = p.songs.map(id => byId.get(id)).filter(Boolean); ctx = p.songs.slice();
    m.innerHTML = `<button class="ghost" data-act="plback">← Back</button><h1 style="margin-top:12px">${esc(p.name)}</h1><p class="tag">Drag &amp; drop se reorder karo (doubly linked list backend mein)</p>` +
      (songs.map((s, i) => row(s, { drag: true, i, rm: true })).join('') || '<p class="mu">Empty — Library se ＋ dabake songs add karo</p>');
    let from = null;
    m.querySelectorAll('.row[draggable]').forEach(r => {
      r.ondragstart = () => { from = +r.dataset.i; r.classList.add('drag'); };
      r.ondragend = () => r.classList.remove('drag');
      r.ondragover = e => e.preventDefault();
      r.ondrop = safe(async () => {
        const to = +r.dataset.i; if (from === null || from === to) return;
        await api(`/playlists/${p.id}/reorder`, { method: 'POST', body: { from, to } }); await renderPls(); markNow();
      });
    });
    return;
  }
  const moods = [...new Set(all.map(s => s.mood).filter(Boolean))];
  m.innerHTML = `<h1>Playlists</h1><div class="top"><input id="pn" placeholder="New playlist name"><button class="btn" id="pc">Create</button></div>
<div class="top"><select id="sm" style="flex:1">${moods.map(x => `<option>${esc(x)}</option>`).join('')}</select><button class="ghost" id="sg2" style="white-space:nowrap">✨ Smart playlist</button></div>
${pls.map(p => `<div class="pl"><div><b>${esc(p.name)}</b><div class="tag">${p.songs.length} songs</div></div><div>
<button class="ghost" data-act="plopen" data-pid="${p.id}">Open</button> <button class="ghost" data-act="plplay" data-pid="${p.id}">▶</button>
<button class="ghost" data-act="plren" data-pid="${p.id}">✎</button> <button class="ghost" data-act="pldel" data-pid="${p.id}">🗑</button></div></div>`).join('')}`;
  const create = safe(async () => {
    const n = $('#pn').value.trim(); if (!n) return;
    await api('/playlists', { method: 'POST', body: { name: n } }); await renderPls();
  });
  $('#pc').onclick = create; $('#pn').onkeydown = e => { if (e.key === 'Enter') create(); };
  $('#sg2').onclick = safe(async () => {
    const mood = $('#sm').value; if (!mood) return toast('Pehle songs mein mood chahiye');
    await api('/playlists/smart', { method: 'POST', body: { mood } }); await renderPls();
  });
}

/* ---------- QUEUE & HISTORY ---------- */
async function renderQ() {
  await loadQueue();
  $('#main').innerHTML = `<h1>Queue</h1><p class="tag">Server-side queue (linked list + cursor)${Q.items.length ? ' · <a href="#" class="c" data-act="qclear">Clear</a>' : ''}</p>` +
    (Q.items.map((id, i) => byId.get(id) && `<div class="row ${i === Q.index ? 'now' : ''}"><span class="tag">${i + 1}</span><div><div class="t">${esc(byId.get(id).title)}</div><div class="tag">${esc(byId.get(id).artist)}</div></div><span></span><span></span><span class="acts"><button class="ghost" data-act="qplay" data-i="${i}">▶</button> <button class="ghost" data-act="qrm" data-i="${i}">✕</button></span></div>`).filter(Boolean).join('') || '<p class="mu">Queue khaali hai</p>');
}
async function renderHist() {
  const d = await api('/history'); ctx = d.songs.map(s => s.id);
  $('#main').innerHTML = `<h1>Recently played</h1><p class="tag">Stack (LIFO) — latest song sabse upar</p>` + (d.songs.map(s => row(s)).join('') || '<p class="mu">Abhi kuch play nahi kiya</p>');
}

/* ---------- ADMIN / PROFILE ---------- */
async function renderAdm() {
  const [st, us] = await Promise.all([api('/admin/stats'), api('/admin/users')]);
  const up = s => { const x = s | 0; return Math.floor(x / 3600) + 'h ' + Math.floor(x % 3600 / 60) + 'm ' + (x % 60) + 's'; };
  await loadSongs();
  $('#main').innerHTML = `<h1>Admin Dashboard</h1>
<div class="cards"><div class="stat"><b>${st.users}</b>Users</div><div class="stat"><b>${st.songs}</b>Songs</div><div class="stat"><b>${st.playlists}</b>Playlists</div><div class="stat"><b>${st.plays}</b>Total plays</div></div>
<h2>System Health</h2><div class="mini"><div class="stat"><b>${up(st.system.uptime_s)}</b>Uptime</div><div class="stat"><b>${st.system.requests}</b>API requests</div><div class="stat"><b>${st.system.avl_height}</b>AVL tree height</div><div class="stat"><b>${st.system.graph_edges}</b>Similarity graph edges</div></div>
<h2 class="sub">Top songs (max-heap)</h2>${st.top.map((s, i) => `<div class="pl"><span>${i + 1}. <b>${esc(s.title)}</b> <span class="tag">${esc(s.artist)}</span></span><span class="badge">${s.plays} play${s.plays === 1 ? "" : "s"}</span></div>`).join('') || '<p class="mu">No data</p>'}
<h2 class="sub">User Management</h2>${us.users.map(u => `<div class="pl"><div><b>${esc(u.username)}</b> <span class="tag">${esc(u.email)} · ${u.role}</span></div>${u.role === 'admin' ? '' : `<button class="ghost" data-act="delusr" data-id="${u.id}">Remove</button>`}</div>`).join('')}
<h2 class="sub">Content Moderation</h2>${all.filter(s => s.uploaded).map(s => `<div class="pl"><span>${esc(s.title)} <span class="tag">${esc(s.artist)}</span></span><button class="ghost" data-act="delsong" data-id="${s.id}">Delete</button></div>`).join('') || '<p class="mu">Koi uploaded song nahi</p>'}`;
}
function renderSet() {
  $('#main').innerHTML = `<h1>Profile &amp; Preferences</h1><div class="stat" style="max-width:380px"><b style="font-size:20px">${esc(user.username)}</b><div class="tag">${esc(user.email)} · ${user.role}</div><p>Theme</p><select id="th"><option value="">System</option><option value="dark">Dark</option><option value="light">Light</option></select></div>`;
  $('#th').value = localStorage.getItem('mp_theme') || '';
  $('#th').onchange = e => { localStorage.setItem('mp_theme', e.target.value); applyTheme(); };
}
function applyTheme() { const t = localStorage.getItem('mp_theme'); t ? document.documentElement.dataset.theme = t : delete document.documentElement.dataset.theme; }

/* ---------- ACTIONS (event delegation) ---------- */
function openDlg(html, w) { const d = $('#dlg'); d.style.maxWidth = w || '320px'; d.innerHTML = html; if (!d.open) d.showModal(); }
const A = {
  play: id => playFrom(id, ctx),
  enq: async id => { const was = Q.items.length; Q = await api('/queue/add', { method: 'POST', body: { song_id: id } }); toast('Queue mein add ho gaya', true); if (!was) loadCurrent(); },
  add: id => openDlg(`<h2>Add to playlist</h2>${pls.map(p => `<button class="ghost" style="width:100%;margin-bottom:6px" data-act="doadd" data-pid="${p.id}" data-id="${id}">${esc(p.name)}</button>`).join('')}<button class="ghost" style="width:100%" data-act="close">Cancel</button>`),
  doadd: async (id, b) => { const d = await api(`/playlists/${b.dataset.pid}/songs`, { method: 'POST', body: { song_id: id } }); $('#dlg').close(); toast(d.added ? 'Playlist mein add ho gaya' : 'Pehle se playlist mein hai', d.added); await loadPls(); },
  close: () => $('#dlg').close(),
  sim: async id => {
    const d = await api(`/songs/${id}/recommend?limit=6`); d.songs.forEach(s => byId.set(s.id, s)); ctx = d.songs.map(s => s.id);
    openDlg(`<h2>Similar to “${esc(byId.get(id)?.title)}”</h2>${d.songs.map(s => row(s)).join('') || '<p class="mu">Koi similar song nahi mila</p>'}<button class="ghost" style="width:100%;margin-top:8px" data-act="close">Close</button>`, '620px');
  },
  rm: async id => { await api(`/playlists/${plView}/songs/${id}`, { method: 'DELETE' }); await renderPls(); markNow(); },
  genre: (_, b) => { filter = b.dataset.g; renderLib(); },
  plopen: (_, b) => { plView = +b.dataset.pid; render(); },
  plback: () => { plView = null; render(); },
  plplay: (_, b) => { const p = pls.find(x => x.id === +b.dataset.pid); if (p && p.songs.length) playFrom(p.songs[0], p.songs); else toast('Playlist khaali hai'); },
  plren: async (_, b) => { const p = pls.find(x => x.id === +b.dataset.pid), n = prompt('Rename playlist', p.name); if (n && n.trim()) { await api('/playlists/' + p.id, { method: 'PUT', body: { name: n } }); await renderPls(); } },
  pldel: async (_, b) => { if (confirm('Delete playlist?')) { await api('/playlists/' + b.dataset.pid, { method: 'DELETE' }); await renderPls(); } },
  qplay: async (_, b) => { Q = await api('/queue/set', { method: 'POST', body: { ids: Q.items, index: +b.dataset.i } }); loadCurrent(); renderQ(); },
  qrm: async (_, b) => { Q = await api('/queue/remove', { method: 'POST', body: { index: +b.dataset.i } }); renderQ(); },
  qclear: async (_, b, e) => { e.preventDefault(); Q = await api('/queue', { method: 'DELETE' }); renderQ(); },
  delusr: async id => { if (confirm('Remove user?')) { await api('/admin/users/' + id, { method: 'DELETE' }); await renderAdm(); } },
  delsong: async id => { if (confirm('Delete song?')) { await api('/songs/' + id, { method: 'DELETE' }); await loadSongs(); await renderAdm(); } },
};
document.addEventListener('click', safe(async e => {
  const b = e.target.closest('[data-act]');
  if (b && A[b.dataset.act]) await A[b.dataset.act](+b.dataset.id, b, e);
}));

/* ---------- PLAYER ---------- */
async function playFrom(id, list) {
  if (Q.items[Q.index] === id && au.src) { au.paused ? au.play() : au.pause(); return; }
  let ids = list.slice(0, 2000), idx = ids.indexOf(id);
  if (idx < 0) { ids = [id]; idx = 0; }
  Q = await api('/queue/set', { method: 'POST', body: { ids, index: idx } });
  loadCurrent();
}
function loadCurrent() {
  const id = Q.items[Q.index], s = byId.get(id);
  if (!s) return;
  au.src = `${BASE}/api/songs/${id}/stream`; au.play().catch(() => {});
  $('#nt').textContent = s.title; $('#na').textContent = s.artist; markNow();
  api(`/songs/${id}/play`, { method: 'POST' }).then(d => { s.plays = d.plays; }).catch(() => {});
}
async function step(dir) {
  const before = Q.index;
  Q = await api('/queue/' + dir, { method: 'POST' });
  if (Q.index !== before) { loadCurrent(); return true; }
  return false;
}
$('#pp').onclick = () => { if (au.src) au.paused ? au.play() : au.pause(); };
$('#nx').onclick = safe(async () => { await step('next'); });
$('#pv').onclick = safe(async () => {
  if (au.currentTime > 3) { au.currentTime = 0; return; }
  if (await step('prev')) return;
  const d = await api('/history/back', { method: 'POST' });          // stack pop: previously played song
  if (d.songs.length) { d.songs.forEach(s => byId.set(s.id, s)); Q = await api('/queue/set', { method: 'POST', body: { ids: [d.songs[0].id], index: 0 } }); loadCurrent(); }
  else au.currentTime = 0;
});
au.onplay = au.onpause = () => { $('#pp').textContent = au.paused ? '▶' : '❚❚'; markNow(); };
au.onended = safe(async () => { await step('next'); });
au.onerror = () => { if (au.src) toast('Audio load nahi ho paya'); };
au.ontimeupdate = () => { $('#cu').textContent = fmt(au.currentTime); $('#du').textContent = fmt(au.duration); $('#seek').value = au.duration ? au.currentTime / au.duration * 100 : 0; };
$('#seek').oninput = e => { if (au.duration) au.currentTime = e.target.value / 100 * au.duration; };
$('#vol').oninput = e => au.volume = e.target.value;
let eqOn = false, actx, bf;
$('#eq').onclick = () => {
  try {
    if (!actx) { actx = new AudioContext(); const s = actx.createMediaElementSource(au); bf = actx.createBiquadFilter(); bf.type = 'lowshelf'; bf.frequency.value = 200; s.connect(bf); bf.connect(actx.destination); }
    actx.resume(); eqOn = !eqOn; bf.gain.value = eqOn ? 10 : 0; $('#eq').style.borderColor = eqOn ? 'var(--c)' : '';
  } catch (e) { toast('EQ is browser mein available nahi'); }
};
document.addEventListener('keydown', e => { if (e.code === 'Space' && !/INPUT|SELECT|TEXTAREA|BUTTON/.test(e.target.tagName) && !$('#app').hidden) { e.preventDefault(); $('#pp').click(); } });

/* ---------- INIT ---------- */
applyTheme();
(async () => {
  if (!token) return;
  try { user = (await api('/me')).user; await boot(); } catch (e) { resetSession(); }
})();
