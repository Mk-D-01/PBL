/* MapGen web front end: canvas map renderer, search, routes, admin CRUD. */
"use strict";

const $ = (id) => document.getElementById(id);
const state = {
  role: "guest",
  map: null,          // {locations:[], connections:[]}
  selected: null,     // selected location object
  view: { x: 0, y: 0, zoom: 1 },   // pan offset + zoom
  dragging: false,
  dragStart: null,
  // --- map designer state (admin only) ---
  mode: "view",       // "view" | "place" | "connect"
  connectFrom: null,  // first location clicked in Connect mode
  dragLoc: null,      // location being dragged on the canvas
  dragMoved: false,
  mouse: { x: 0, y: 0 },
};

/* ---------------- helpers ---------------- */

async function api(path, options = {}) {
  const res = await fetch(path, {
    headers: { "Content-Type": "application/json" },
    ...options,
  });
  const text = await res.text();
  let body = {};
  try { body = text ? JSON.parse(text) : {}; } catch { body = { error: text }; }
  if (!res.ok) throw new Error(body.error || `HTTP ${res.status}`);
  return body;
}

const TYPE_COLORS = { G: "#f0b429", B: "#4f9cff", F: "#57c46b" };
const TYPE_NAMES = { G: "Gate", B: "Building", F: "Facility" };

/* ---------------- canvas renderer ---------------- */

const canvas = $("mapCanvas");
const ctx = canvas.getContext("2d");

// The canvas is stretched by CSS, so its on-screen size differs from its pixel size
// (canvas.width/height). Mouse events are in screen pixels; all drawing/hit-test math
// is in canvas pixels. Always go through canvasPos() so the two agree.
function canvasPos(e) {
  const r = canvas.getBoundingClientRect();
  return {
    x: (e.clientX - r.left - canvas.clientLeft) * canvas.width / canvas.clientWidth,
    y: (e.clientY - r.top - canvas.clientTop) * canvas.height / canvas.clientHeight,
  };
}

// Keep the pixel buffer equal to the displayed size so nothing is stretched or blurry.
function fitCanvas() {
  if (canvas.clientWidth && canvas.clientHeight &&
      (canvas.width !== canvas.clientWidth || canvas.height !== canvas.clientHeight)) {
    canvas.width = canvas.clientWidth;
    canvas.height = canvas.clientHeight;
  }
}

// Campus world is roughly 62x20 grid units; pad and scale to fit.
function worldToScreen(wx, wy) {
  const v = state.view;
  return {
    x: 30 * v.zoom + (wx - 0) * 15 * v.zoom + v.x,
    y: 25 * v.zoom + (wy - 0) * 15 * v.zoom + v.y,
  };
}

function screenToWorld(sx, sy) {
  const v = state.view;
  return {
    x: (sx - 30 * v.zoom - v.x) / (15 * v.zoom),
    y: (sy - 25 * v.zoom - v.y) / (15 * v.zoom),
  };
}

function drawMap() {
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  if (!state.map) return;
  const v = state.view;

  // usable area: locations outside it are clamped by the server, so show where it ends
  const b = state.map.bounds;
  if (b) {
    const tl = worldToScreen(b.minX - 0.5, b.minY - 0.5);
    const br = worldToScreen(b.maxX + 0.5, b.maxY + 0.5);
    ctx.fillStyle = "rgba(255,255,255,0.025)";
    ctx.fillRect(tl.x, tl.y, br.x - tl.x, br.y - tl.y);
    ctx.setLineDash([5, 5]);
    ctx.strokeStyle = "#2a3345";
    ctx.lineWidth = 1;
    ctx.strokeRect(tl.x, tl.y, br.x - tl.x, br.y - tl.y);
    ctx.setLineDash([]);
  }

  // walkways
  ctx.strokeStyle = "#33415c";
  ctx.lineWidth = 2;
  for (const c of state.map.connections) {
    const a = state.map.locations.find((l) => l.id === c.from);
    const b = state.map.locations.find((l) => l.id === c.to);
    if (!a || !b) continue;
    const pa = worldToScreen(a.x, a.y);
    const pb = worldToScreen(b.x, b.y);
    ctx.beginPath();
    ctx.moveTo(pa.x, pa.y);
    ctx.lineTo(pb.x, pb.y);
    ctx.stroke();
    // distance label at midpoint
    const mx = (pa.x + pb.x) / 2, my = (pa.y + pb.y) / 2;
    ctx.fillStyle = "#8a94a6";
    ctx.font = "10px monospace";
    ctx.fillText(`${c.d}m`, mx + 3, my - 3);
  }

  // locations
  const radius = 9 * v.zoom;
  for (const loc of state.map.locations) {
    const p = worldToScreen(loc.x, loc.y);
    const isSel = state.selected && state.selected.id === loc.id;
    ctx.beginPath();
    ctx.arc(p.x, p.y, Math.max(radius, 5), 0, Math.PI * 2);
    ctx.fillStyle = TYPE_COLORS[loc.type] || "#888";
    ctx.fill();
    if (isSel) {
      ctx.strokeStyle = "#fff";
      ctx.lineWidth = 3;
      ctx.stroke();
    }
    ctx.fillStyle = "#e6ebf2";
    ctx.font = `${Math.max(11, 12 * v.zoom)}px system-ui`;
    ctx.fillText(loc.name, p.x + radius + 3, p.y + 4);
  }

  // pending connection preview in Connect mode
  if (state.mode === "connect" && state.connectFrom) {
    const a = worldToScreen(state.connectFrom.x, state.connectFrom.y);
    ctx.setLineDash([6, 5]);
    ctx.strokeStyle = "#4f9cff";
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(a.x, a.y);
    ctx.lineTo(state.mouse.x, state.mouse.y);
    ctx.stroke();
    ctx.setLineDash([]);
    ctx.strokeStyle = "#fff";
    ctx.lineWidth = 3;
    ctx.beginPath();
    ctx.arc(a.x, a.y, Math.max(11 * v.zoom, 7), 0, Math.PI * 2);
    ctx.stroke();
  }
}

/* ---------------- map data ---------------- */

async function refreshMap() {
  state.map = await api("/api/map");
  await refreshMapList();
  drawMap();
  $("infoOut").textContent = "";
}

// Rebuilds the header dropdown from the server and shows the unsaved-changes dot.
async function refreshMapList() {
  const r = await api("/api/maps");
  const sel = $("mapSelect");
  sel.innerHTML = "";
  for (const m of r.maps) {
    const opt = document.createElement("option");
    opt.value = m.name;
    opt.textContent = m.saved ? m.name : `${m.name} (unsaved)`;
    sel.appendChild(opt);
  }
  sel.value = r.active;
  state.mapName = r.active;
  state.dirty = r.dirty;
  $("mapDirty").classList.toggle("hidden", !r.dirty);
}

// Zoom/pan so the whole map is visible (maps can be anywhere from 20x10 to 200x100).
function fitView() {
  const m = state.map;
  if (!m) return;
  const contentW = 30 + 15 * m.width + 30;
  const contentH = 25 + 15 * m.height + 25;
  const zoom = Math.min(canvas.width / contentW, canvas.height / contentH);
  state.view.zoom = Math.min(4, Math.max(0.1, zoom));
  state.view.x = 0;
  state.view.y = 0;
  drawMap();
}

// Snap a world position into the area the server accepts, so what you see is what you get.
function clampToBounds(x, y) {
  const b = state.map && state.map.bounds;
  if (!b) return { x: Math.max(0, x), y: Math.max(0, y) };
  return {
    x: Math.min(b.maxX, Math.max(b.minX, x)),
    y: Math.min(b.maxY, Math.max(b.minY, y)),
  };
}

function selectLocation(loc) {
  state.selected = loc;
  $("selectedInfo").textContent = loc
    ? `#${loc.id} ${loc.name} — ${TYPE_NAMES[loc.type]} — ${loc.desc}`
    : "Click a location on the map…";
  $("renameBtn").disabled = !loc || state.role !== "admin";
  $("moveBtn").disabled = !loc || state.role !== "admin";
  $("deleteLocBtn").disabled = !loc || state.role !== "admin";
  drawMap();
}

/* ---------------- search ---------------- */

let searchTimer = null;
$("searchBox").addEventListener("input", () => {
  clearTimeout(searchTimer);
  searchTimer = setTimeout(runSearch, 200);
});

async function runSearch() {
  const q = $("searchBox").value.trim();
  const list = $("searchResults");
  if (!q) { list.innerHTML = ""; return; }
  try {
    const { results } = await api(`/api/search?q=${encodeURIComponent(q)}`);
    list.innerHTML = "";
    for (const r of results) {
      const li = document.createElement("li");
      li.innerHTML = `${r.name} <span class="type">#${r.id} ${TYPE_NAMES[r.type] || ""}</span>`;
      li.onclick = () => selectLocation(r);
      list.appendChild(li);
    }
    if (!results.length) {
      list.innerHTML = `<li class="muted">no matches for “${q}”</li>`;
    }
  } catch (e) {
    list.innerHTML = `<li class="muted">${e.message}</li>`;
  }
}

/* ---------------- routes ---------------- */

$("routeBtn").addEventListener("click", async () => {
  const from = $("routeFrom").value.trim();
  const to = $("routeTo").value.trim();
  const metric = document.querySelector('input[name="metric"]:checked').value;
  const out = $("routeResult");
  if (!from || !to) { out.innerHTML = `<span class="err">enter source and destination</span>`; return; }
  try {
    const r = await api(`/api/route?from=${encodeURIComponent(from)}&to=${encodeURIComponent(to)}&metric=${metric}`);
    if (!r.reachable) {
      out.innerHTML = `<span class="err">no route found${r.exists ? "" : " (DFS confirms disconnected)"}</span>`;
      return;
    }
    out.innerHTML =
      `<div class="ok">${r.path.join(" → ")}</div>` +
      `<div class="muted">${r.hops} stops · ${r.totalDistance} m total</div>`;
  } catch (e) {
    out.innerHTML = `<span class="err">${e.message}</span>`;
  }
});

/* ---------------- auth ---------------- */

async function whoami() {
  try {
    const r = await api("/api/whoami");
    state.role = r.role;
    $("whoami").textContent = r.username ? `${r.username} (${r.role})` : r.role;
    $("adminPanel").classList.toggle("hidden", r.role !== "admin");
    $("loginForm").classList.toggle("hidden", r.role !== "guest");
    $("logoutBtn").classList.toggle("hidden", r.role === "guest");
    $("mapSelect").disabled = r.role !== "admin";  // the active map is global, so only admins switch it
    if (r.role !== "admin" && state.mode !== "view") setMode("view");
    selectLocation(state.selected);
  } catch { /* server offline */ }
}

$("loginForm").addEventListener("submit", async (e) => {
  e.preventDefault();
  try {
    await api("/login", {
      method: "POST",
      body: JSON.stringify({ username: $("loginUser").value, password: $("loginPass").value }),
    });
    $("loginPass").value = "";
    await whoami();
  } catch (err) {
    $("whoami").textContent = `login failed: ${err.message}`;
  }
});

$("logoutBtn").addEventListener("click", async () => {
  await api("/logout", { method: "POST" }).catch(() => {});
  await whoami();
});

/* ---------------- maps ---------------- */

function mapMessage(text, isError) {
  const el = $("mapMsg");
  el.textContent = text;
  el.className = isError ? "err small" : "muted small";
}

// Shows a freshly activated map: forget the old selection and fit it into view.
async function showActiveMap() {
  state.selected = null;
  state.connectFrom = null;
  await refreshMap();
  fitView();
  selectLocation(null);
}

function confirmDiscardUnsaved() {
  return !state.dirty || confirm(`"${state.mapName}" has unsaved changes. Continue and lose them?`);
}

$("mapSelect").addEventListener("change", async (e) => {
  const target = e.target.value;
  if (!confirmDiscardUnsaved()) { e.target.value = state.mapName; return; }
  try {
    await api("/api/maps/select", { method: "POST", body: JSON.stringify({ name: target }) });
    await showActiveMap();
    mapMessage(`Switched to "${target}".`);
  } catch (err) {
    mapMessage(err.message, true);
    await refreshMapList().catch(() => {});
  }
});

$("createMapBtn").addEventListener("click", async () => {
  if (!confirmDiscardUnsaved()) return;
  const name = $("newMapName").value.trim();
  const width = parseInt($("newMapW").value, 10);
  const height = parseInt($("newMapH").value, 10);
  try {
    await api("/api/maps", { method: "POST", body: JSON.stringify({ name, width, height }) });
    $("newMapName").value = "";
    await showActiveMap();
    mapMessage(`Created "${name}" (${width}x${height}). Use Place mode to add locations, then Save.`);
  } catch (err) {
    mapMessage(err.message, true);
  }
});

async function saveMap(asName) {
  try {
    const body = asName ? JSON.stringify({ name: asName }) : undefined;
    const r = await api("/api/maps/save", { method: "POST", body });
    await refreshMap();
    mapMessage(`Saved "${r.name}".`);
  } catch (err) {
    mapMessage(err.message, true);
  }
}

$("saveMapBtn").addEventListener("click", () => saveMap(null));
$("saveAsBtn").addEventListener("click", () => {
  const name = $("newMapName").value.trim();
  if (!name) { mapMessage("Type the new name in the Name box first.", true); return; }
  saveMap(name).then(() => { $("newMapName").value = ""; });
});

$("fitBtn").addEventListener("click", fitView);

/* ---------------- admin actions ---------------- */

$("addLocBtn").addEventListener("click", async () => {
  try {
    await api("/api/locations", {
      method: "POST",
      body: JSON.stringify({
        name: $("locName").value.trim(),
        type: $("locType").value,
        detail: $("locDetail").value.trim() || "campus spot",
        x: parseInt($("locX").value, 10),
        y: parseInt($("locY").value, 10),
      }),
    });
    $("locName").value = "";
    await refreshMap();
  } catch (e) { alert(e.message); }
});

$("addConnBtn").addEventListener("click", async () => {
  try {
    await api("/api/connections", {
      method: "POST",
      body: JSON.stringify({
        from: parseInt($("connFrom").value, 10),
        to: parseInt($("connTo").value, 10),
        d: parseInt($("connDist").value, 10),
        label: $("connLabel").value.trim() || "walkway",
      }),
    });
    $("connLabel").value = "";
    await refreshMap();
  } catch (e) { alert(e.message); }
});

$("delConnBtn").addEventListener("click", async () => {
  try {
    const f = $("connFrom").value, t = $("connTo").value;
    await api(`/api/connections?from=${f}&to=${t}`, { method: "DELETE" });
    await refreshMap();
  } catch (e) { alert(e.message); }
});

$("renameBtn").addEventListener("click", async () => {
  if (!state.selected) return;
  const name = prompt("New name:", state.selected.name);
  if (!name) return;
  try {
    await api("/api/locations", { method: "PUT", body: JSON.stringify({ id: state.selected.id, name }) });
    selectLocation(null);
    await refreshMap();
  } catch (e) { alert(e.message); }
});

$("moveBtn").addEventListener("click", async () => {
  if (!state.selected) return;
  const x = parseInt(prompt("New x:", state.selected.x), 10);
  const y = parseInt(prompt("New y:", state.selected.y), 10);
  if (Number.isNaN(x) || Number.isNaN(y)) return;
  try {
    await api("/api/locations", { method: "PUT", body: JSON.stringify({ id: state.selected.id, x, y }) });
    await refreshMap();
  } catch (e) { alert(e.message); }
});

$("deleteLocBtn").addEventListener("click", async () => {
  if (!state.selected || !confirm(`Delete ${state.selected.name}?`)) return;
  try {
    await api(`/api/locations/${state.selected.id}`, { method: "DELETE" });
    selectLocation(null);
    await refreshMap();
  } catch (e) { alert(e.message); }
});

$("generateBtn").addEventListener("click", async () => {
  if (!confirm("Regenerate the default campus? Custom edits are lost.")) return;
  await api("/api/generate", { method: "POST" }).catch((e) => alert(e.message));
  await refreshMap();
});

/* ---------------- info + canvas events ---------------- */

$("infoBtn").addEventListener("click", async () => {
  try {
    const r = await api("/api/info");
    $("infoOut").textContent = r.info;
  } catch (e) {
    $("infoOut").textContent = e.message;
  }
});

canvas.addEventListener("mousedown", (e) => {
  const p = canvasPos(e);
  const hit = hitTest(p.x, p.y);
  if (state.role === "admin" && hit) {
    // admins grab the location itself (drag to move, click to select/act)
    state.dragLoc = hit;
    state.dragMoved = false;
  } else {
    state.dragging = true;
    state.dragStart = { x: p.x, y: p.y, vx: state.view.x, vy: state.view.y };
  }
});

canvas.addEventListener("mousemove", (e) => {
  const p = canvasPos(e);
  state.mouse = { x: p.x, y: p.y };
  if (state.dragLoc) {
    const w = screenToWorld(p.x, p.y);
    const c = clampToBounds(Math.round(w.x), Math.round(w.y));
    state.dragLoc.x = c.x;
    state.dragLoc.y = c.y;
    state.dragMoved = true;
    drawMap();
    return;
  }
  if (!state.dragging) {
    if (state.mode === "connect" && state.connectFrom) drawMap();
    return;
  }
  state.view.x = state.dragStart.vx + (p.x - state.dragStart.x);
  state.view.y = state.dragStart.vy + (p.y - state.dragStart.y);
  drawMap();
});

canvas.addEventListener("mouseup", (e) => {
  const p = canvasPos(e);
  if (state.dragLoc) {
    const loc = state.dragLoc;
    state.dragLoc = null;
    if (state.dragMoved) {
      api("/api/locations", { method: "PUT", body: JSON.stringify({ id: loc.id, x: loc.x, y: loc.y }) })
        .then(refreshMap)
        .catch((err) => alert(err.message));
    } else {
      handleMapClick(p.x, p.y);  // treat as a click
    }
    return;
  }
  if (!state.dragging) return;
  state.dragging = false;
  const moved = Math.hypot(p.x - state.dragStart.x, p.y - state.dragStart.y);
  if (moved <= 4) handleMapClick(p.x, p.y);
});

canvas.addEventListener("wheel", (e) => {
  e.preventDefault();
  const p = canvasPos(e);
  const factor = e.deltaY < 0 ? 1.1 : 1 / 1.1;
  const v = state.view;
  const newZoom = Math.min(4, Math.max(0.1, v.zoom * factor));
  // keep the point under the cursor fixed while zooming
  const wx = (p.x - v.x) / v.zoom, wy = (p.y - v.y) / v.zoom;
  v.zoom = newZoom;
  v.x = p.x - wx * newZoom;
  v.y = p.y - wy * newZoom;
  drawMap();
}, { passive: false });

window.addEventListener("resize", () => { fitCanvas(); drawMap(); });

function hitTest(sx, sy) {
  if (!state.map) return null;
  const radius = Math.max(9 * state.view.zoom, 5) + 4;  // hit tolerance
  let best = null, bestDist = Infinity;
  for (const loc of state.map.locations) {
    const p = worldToScreen(loc.x, loc.y);
    const d = Math.hypot(p.x - sx, p.y - sy);
    if (d < radius && d < bestDist) { best = loc; bestDist = d; }
  }
  return best;
}

function handleMapClick(sx, sy) {
  const hit = hitTest(sx, sy);
  if (state.role === "admin" && state.mode === "place") {
    placeLocationAt(sx, sy);
    return;
  }
  if (state.role === "admin" && state.mode === "connect") {
    if (!hit) return;
    if (!state.connectFrom) {
      state.connectFrom = hit;
      $("modeHint").textContent = `Connect from ${hit.name} — now click the second location.`;
    } else if (hit.id === state.connectFrom.id) {
      state.connectFrom = null;
      $("modeHint").textContent = "Connect cancelled. Click the first location.";
    } else {
      finishConnect(hit);
      return;
    }
    drawMap();
    return;
  }
  selectLocation(hit);
}

/* ---------------- map designer (admin) ---------------- */

async function placeLocationAt(sx, sy) {
  const name = $("locName").value.trim();
  if (!name) {
    alert("Type a name in the Add Location form first, then click the map.");
    return;
  }
  const w = screenToWorld(sx, sy);
  const { x, y } = clampToBounds(Math.round(w.x), Math.round(w.y));
  try {
    await api("/api/locations", {
      method: "POST",
      body: JSON.stringify({
        name,
        type: $("locType").value,
        detail: $("locDetail").value.trim() || "campus spot",
        x, y,
      }),
    });
    $("locName").value = "";
    $("locX").value = x;
    $("locY").value = y;
    await refreshMap();
  } catch (e) { alert(e.message); }
}

async function finishConnect(second) {
  const a = state.connectFrom, b = second;
  state.connectFrom = null;
  const dist = prompt(`Distance ${a.name} ↔ ${b.name} (metres):`, "50");
  if (dist === null) {
    $("modeHint").textContent = "Connect cancelled. Click the first location.";
    drawMap();
    return;
  }
  const d = Math.max(1, parseInt(dist, 10) || 50);
  const label = prompt("Walkway label:", "walkway") || "walkway";
  try {
    await api("/api/connections", {
      method: "POST",
      body: JSON.stringify({ from: a.id, to: b.id, d, label }),
    });
    $("connFrom").value = a.id;
    $("connTo").value = b.id;
    $("connDist").value = d;
    await refreshMap();
    $("modeHint").textContent = `Connected ${a.name} ↔ ${b.name}. Click the first location for the next walkway.`;
  } catch (e) {
    alert(e.message);
    $("modeHint").textContent = "Connect failed (duplicate edge?). Click the first location.";
  }
  drawMap();
}

const MODE_HINTS = {
  view: "Pan mode: drag empty space to move the map, scroll to zoom, click a location to select it.",
  place: "Place mode: fill the Add Location form, then click the map to drop the location there.",
  connect: "Connect mode: click the first location, then the second — you'll be asked for distance and label.",
};

function setMode(mode) {
  if (state.role !== "admin" && mode !== "view") mode = "view";
  state.mode = mode;
  state.connectFrom = null;
  $("modeHint").textContent = MODE_HINTS[mode];
  const badge = $("modeBadge");
  badge.classList.toggle("hidden", mode === "view");
  badge.textContent = mode === "place" ? "📍 Place mode — click the map"
                    : mode === "connect" ? "🔗 Connect mode — click two locations" : "";
  for (const btn of document.querySelectorAll("#modeBar .mode")) {
    const id = "mode" + mode[0].toUpperCase() + mode.slice(1);
    btn.classList.toggle("active", btn.id === id);
  }
  canvas.style.cursor = mode === "view" ? "pointer" : "crosshair";
  drawMap();
}

$("modeView").addEventListener("click", () => setMode("view"));
$("modePlace").addEventListener("click", () => setMode("place"));
$("modeConnect").addEventListener("click", () => setMode("connect"));

/* ---------------- boot ---------------- */

fitCanvas();
refreshMap().then(() => { fitView(); return whoami(); }).catch((e) => {
  $("whoami").textContent = "server unreachable";
  console.error(e);
});
