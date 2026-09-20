# Implementation Plan — Web Front End for MapGen Engine

Extension of the existing native C++ MapGen Engine (Campus Locator and Navigation).
The engine, its custom data structures, and the console app remain untouched;
a small HTTP layer written in C++ serves a browser front end.

## 1. Architecture

```
 Browser (HTML/CSS/JS canvas)            mapgen_web.exe (native C++)
┌───────────────────────────┐            ┌────────────────────────────────────┐
│ index.html, style.css,    │  HTTP/JSON │ HttpServer (Winsock/BSD sockets)   │
│ app.js (canvas renderer,  │◄──────────►│   └─ ApiController                 │
│ fetch calls)              │            │       ├─ /api/...  (user, admin)   │
└───────────────────────────┘            │       ├─ /login, / (static files)  │
                                         │   └─ WebSession (role: user/admin) │
                                         │   └─ CampusMap / MapGenerator /    │
                                         │      Navigation  (existing engine) │
                                         └────────────────────────────────────┘
```

- One `CampusMap` instance lives in the server process; **all HTTP handlers run on
  the main loop** (single-threaded, `select()`-based or sequential accept loop),
  so the engine needs no locking.
- New code: `include/web/*`, `src/web/*`. No changes to `src/` engine files
  except `CMake`/`Makefile` target additions.

## 2. Front-end pages

| File | Purpose |
|---|---|
| `web/index.html` | One page, two panes: canvas map + sidebar (search box, results, route panel, admin panel) |
| `web/style.css` | Dark theme, panels, buttons |
| `web/app.js` | Canvas renderer (draws locations as circles, walkways as lines from the adjacency JSON; pan/zoom optional), search-as-you-type, route panel, admin CRUD forms |

## 3. REST API (JSON)

| Method | Path | Role | Purpose |
|---|---|---|---|
| POST | `/login` | any | body `{username, password}`; sets session cookie |
| POST | `/logout` | any | clears session |
| GET | `/api/whoami` | any | `{role}` |
| GET | `/api/map` | user | full map state: locations + connections (for canvas render) |
| GET | `/api/search?q=...` | user | exact + keyword matches (reuse `CampusMap::searchByKeyword`) |
| GET | `/api/route?from=..&to=..&metric=hops\|distance` | user | BFS / Dijkstra result |
| POST | `/api/locations` | **admin** | add location |
| PUT | | **admin** | update name/position |
| DELETE | `/api/locations/{id}` | **admin** | remove location |
| POST | `/api/connections` | **admin** | connect two locations (distance, label) |
| DELETE | `/api/connections?from=&to=` | **admin** | disconnect |
| POST | `/api/generate` | **admin** | generate default campus (option 1) |
| GET | `/api/info` | user | campus stats + adjacency report |

Every admin endpoint returns `401/403` JSON error for user-role sessions.

## 4. Auth model (minimal, good enough for an academic project)

- `HttpServer` parses the `Cookie` header; sessions stored in a
  `ds::HashMap<std::string, Session>` keyed by random session token (custom DS reuse).
- Two seeded accounts: `admin/admin123`, `user/user123`; passwords in plain
  struct + role enum (`Role::{User, Admin}`); no hashing needed for a demo.
- `WebSession` helpers: `currentRole()`, `requireAdmin()` guard wrapper.

## 5. Implementation steps (in build order)

1. **HttpServer** (`src/web/HttpServer.cpp`): blocking accept loop; parse request
   line + headers + body; route to controller; send response; handle GET/POST/PUT/DELETE.
2. **JSON helpers** (`src/web/Json.cpp`): tiny serializer for map/route/search
   responses; tiny parser for admin request bodies (locations/connections).
   (Hand-rolled JSON — keep the "no external deps" story consistent.)
   Hand-rolled JSON parsing/serialization — **this is the riskiest step**, budget extra time.
3. **ApiController** (`src/web/ApiController.cpp`): wire endpoints to
   `CampusMap` / `Navigation` / `MapGenerator` calls; JSON in/out.
4. **Auth & sessions**: login/logout/whoami, session cookie, role guards on admin routes.
5. **Static file serving**: serve `web/` (index.html, style.css, app.js) with
   correct MIME types; `/` → `index.html`.
6. **Front end**: canvas map renderer + search + route panel (user features);
   admin panel (add/update/remove locations, add/remove connections, generate).
   Admin panel hidden unless `whoami` says `role=admin`.
7. **Makefile target** `mapgen_web` + `build.bat` lines; README updates.
8. **Tests**: `tests/web_test.cpp` — raw-socket HTTP requests against a loopback
   server: login, search, route correctness (MainGate→Hostel-A = 290 m), admin guard (403), CRUD round-trip.
9. **Polish**: 404/405 handling, URL decoding for `q`/`from`/`to` query params, 404 on unknown static files.

## 6. Role matrix (final, confirmed)

| Feature | User | Admin |
|---|:-:|:-:|
| View map (canvas + list) | ✔ | ✔ |
| Search locations | ✔ | ✔ |
| Find routes (BFS/Dijkstra/DFS) | ✔ | ✔ |
| Generate default campus | ✖ | ✔ |
| Add / update / remove locations | ✖ | ✔ |
| Add / remove connections | ✖  | ✔ |

## Architecture notes / risks

- **JSON by hand** is the risky part — keep responses flat and field names short.
- Single-threaded server: fine for a viva demo; document as a known limitation.
- Windows sockets need `WSAStartup` guards — isolate in `HttpServer`.
- Ports: use **18080** to avoid clashing with any running dev servers.
- The console `Menu` stays fully functional and independent of the web layer.
- Explicit non-goals for this iteration: file save/load, blocked roads, role management UI.
