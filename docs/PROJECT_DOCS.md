# MapGen Engine — Project Documentation

*The simple, complete guide: what it is, how it works, what it's built with, what every file does, which algorithms are used, and how the work divides across a 6-person team.*

---

## 1. What is this project?

A **campus navigation system** written in native C++. The campus is modelled as a
**graph**: locations (gates, blocks, library, cafeteria…) are *nodes* and the
walkways between them are *weighted edges* (distance in metres).

It answers three simple questions:
1. **Where is it?** — search locations by keyword.
2. **How do I get there?** — find a route with fewest stops (BFS) or shortest distance (Dijkstra).
3. **Can I edit the campus?** — admins add/remove/rename locations and draw walkways, on a visual map designer.

It runs two ways:
- a **console menu** (10 options), and
- a **browser app** served by the same C++ engine (map canvas, search, routes, admin designer).

**Every data structure is written from scratch** — no `std::vector`, `std::map`,
or `std::priority_queue` in the engine. That is the core course requirement.

---

## 2. How it works (30-second version)

```
 Browser (or console)                    C++ engine (one process)
┌──────────────────────┐                ┌──────────────────────────────┐
│ index.html + app.js  │  HTTP / JSON   │ HttpServer → ApiController   │
│ canvas map, forms    │◄──────────────►│      │                        │
│                      │                │      ▼                       │
└──────────────────────┘                │ CampusMap (graph)            │
 Console menu (10 options) ────────────►│      ▼                       │
                                        │ Navigation: BFS/Dijkstra/DFS │
                                        │      ▼                       │
                                        │ ds:: DynamicArray, LinkedList│
                                        │      HashMap, Queue, Stack,  │
                                        │      MinHeap  (all handmade) │
                                        └──────────────────────────────┘
```

- The **server** starts with a demo campus of 12 locations and 20 walkways (seeded by `MapGenerator`).
- Every click in the browser becomes a small HTTP request (`/api/map`, `/api/route`, …).
- The **ApiController** translates those requests into calls on the graph engine and returns JSON.
- The browser draws the graph on an HTML canvas: circles = locations, lines = walkways.
- Admins get designer tools: **click the map to place a location, drag one to move it, click two to connect them**.

---

## 3. Tech stack

| Layer | Technology | Why |
|---|---|---|
| Language | C++17 | course requirement: native, no frameworks |
| Data structures | hand-written templates in `ds::` namespace | mandatory requirement |
| Algorithms | BFS, Dijkstra, DFS, Bresenham, FNV-1a/Murmur hashing | hand-implemented |
| Networking | Winsock/BSD sockets (raw HTTP/1.1 server) | no web framework |
| Data format | hand-rolled JSON parser + writer | no external libraries |
| Front end | vanilla HTML + CSS + JavaScript + Canvas 2D | no build tools, no frameworks |
| Build | `g++` (Makefile + `build.bat`) | one compiler flag set |

Zero third-party dependencies. The whole project compiles to two binaries:
`mapgen.exe` (console) and `mapgen_web.exe` (web server).

---

## 4. Every file explained

### 4.1 Data structures — `include/data_structures/` (the mandatory layer)

| File | What it does |
|---|---|
| `DynamicArray.hpp` | Growable array. Doubles capacity when full, shifts elements manually on insert/remove. Used everywhere as the project's "list". |
| `LinkedList.hpp` | Doubly linked list with raw nodes, iterators, and `removeFirstIf`. Used for adjacency buckets and hash-map chaining. |
| `HashMap.hpp` | Separate-chaining hash table. Buckets are arrays of linked lists; rehashes automatically at 0.75 load factor. Used for name→id lookup, sessions, and the visited/parent/distance sets of every algorithm. |
| `Queue.hpp` | FIFO queue on linked nodes. Powers BFS. |
| `Stack.hpp` | LIFO stack on linked nodes. Powers DFS. |
| `MinHeap.hpp` | Binary min-heap with sift-up/sift-down **and decrease-key** — the priority queue for Dijkstra. |
| `Hash.hpp` | The hash functions: FNV-1a for strings, Murmur3 finalizer for integers. |

### 4.2 Domain model & graph engine — `include/`, `src/`

| File | What it does |
|---|---|
| `Location.hpp` | Abstract base class `Location` (id, name, x, y) with three children: `Building` (floors), `Facility` (service), `Gate` (open/closed). Demonstrates inheritance + virtual polymorphism (`kind()`, `describe()`). |
| `Connection.hpp` | One undirected edge: destination id, distance (metres), walkway label. |
| `Route.hpp` | Result of a route query: path (ids), total distance, hop count, plus the `HeapEntry` used inside Dijkstra's heap. |
| `CampusMap.hpp/.cpp` | **The graph engine.** Owns the id-indexed location table (`DynamicArray`), the name→id `HashMap`, and adjacency (`DynamicArray` of `LinkedList<Connection>`). Implements add/remove/update/search, connect/disconnect (rejects duplicates, self-loops, bad ids), stable-id deletion, the ASCII map (Bresenham lines on a char grid), and the info reports. |
| `MapGenerator.hpp/.cpp` | Seeds the demo campus: 12 placed locations and 20 labelled walkways, ids assigned in creation order (MainGate = 0). Clears the map first so it can be re-run. |

### 4.3 Algorithms — `Navigation.hpp/.cpp`, `Route.hpp`

See §5 below for how each algorithm works.

### 4.4 Console app — `Menu.hpp/.cpp`, `main.cpp`

| File | What it does |
|---|---|
| `Menu.hpp/.cpp` | The 10-option console menu: generate map, display ASCII map, add/remove/update location, search, connect, find route, campus info, exit. Locations are addressable by id **or** name. Detects a non-interactive pipe so scripted demos don't block. |
| `main.cpp` | Entry point; constructs the engine and starts the menu loop. |

### 4.5 Web backend — `include/web/`, `src/web/`, `web_main.cpp`

| File | What it does |
|---|---|
| `HttpServer.hpp/.cpp` | Tiny HTTP/1.1 server on Winsock. Listens on `127.0.0.1:18080`, parses request line + headers + body, one request per connection. Also serves the static front-end files (`.html/.css/.js`) with the right MIME types and path-traversal protection. |
| `Json.hpp/.cpp` | Hand-rolled JSON value type + recursive-descent parser (null/bool/number/string/array/object, `\uXXXX` escapes) and a writer with proper string escaping. Powers all API payloads. |
| `ApiController.hpp/.cpp` | Translates HTTP endpoints into engine calls. **User endpoints** (read-only): `/api/map`, `/api/search?q=`, `/api/route?from=&to=&metric=hops|distance`, `/api/traverse?from=&order=bfs|dfs`, `/api/info`, `/api/whoami`. **Admin endpoints** (session-guarded): `/login`, `/logout`, `/api/generate`, `/api/locations` (POST/PUT/DELETE), `/api/connections` (POST/DELETE). Sessions are tokens stored in the custom `ds::HashMap`; guards return 401 (no login) / 403 (not admin). |
| `web_main.cpp` | Entry point: parses `--port`/`--root` flags, wires controller to server, runs forever. |

### 4.6 Web front end — `web/`

| File | What it does |
|---|---|
| `index.html` | One page: sidebar (login, search, route finder, admin panel, campus info) + canvas. |
| `style.css` | Dark theme, panel layout, mode-bar styles. |
| `app.js` | All behaviour: fetch wrappers, **canvas renderer** (walkways as lines with distance labels, locations as colour-coded circles — gates amber, buildings blue, facilities green), pan (drag) + zoom (wheel), search-as-you-type, route display, and the **admin map designer**: Pan / Place / Connect modes — click to drop a new location at that spot, drag a location to move it (auto-saved), click two locations to create a walkway (asks distance + label). The admin panel only appears for admin sessions. |

### 4.7 Tests — `tests/`

| File | What it does |
|---|---|
| `self_test.cpp` | 91 assertions: every data structure operation, graph CRUD, edge symmetry, BFS/Dijkstra optimality, removal cleanup, scripted UI run. |
| `web_test.cpp` | 41 assertions driven through **real sockets**: login/logout, 401/403 role guards, map/search/route correctness (MainGate→Hostel-A = 290 m), full admin CRUD round-trip, static file serving. |

---

## 5. Algorithms used (simple explanations)

| Algorithm | What it does here | How (plain words) |
|---|---|---|
| **BFS** (fewest stops) | Shortest route by number of walkways. | Explore the graph level by level using a **Queue**; the first time you reach the destination you used the fewest edges. Parents recorded in a `HashMap` rebuild the path. |
| **Dijkstra** (shortest distance) | Shortest route by total metres. | Keep the best-known distance to each node in a **MinHeap**; always expand the closest unfinished node; **decrease-key** when a shorter path is found. Optimal for positive weights (distances). |
| **DFS** (connectivity) | "Is there *any* path at all?" | Go as deep as possible using a **Stack**, backtrack when stuck. Used to confirm reachability and for traversal order. |
| **BFS/DFS traversal** | Visit-order lists from any starting point. | Same two algorithms, just recording the order every node is first visited. |
| **Bresenham line drawing** | ASCII map walkways. | Classic integer-only algorithm that plots the pixels (chars) of a straight line between two grid points. |
| **FNV-1a + Murmur3 hashing** | Fast name→id lookups. | FNV-1a mixes string bytes; Murmur3 finalizer scrambles integer ids; modulo bucket count picks the bucket. Chaining resolves collisions. |
| **Separate chaining + rehash** | `HashMap` internals. | Colliding entries link in a list per bucket; when entries > 75% of buckets, everything rehashes into double the buckets. |
| **Binary heap + decrease-key** | Dijkstra's priority queue. | Array-shaped tree: parent ≤ children; sift-up on insert, sift-down on extract, sift-up after lowering a key. |

---

## 6. The team — 6 niche roles and their share

The project splits naturally into six specialised roles. Each role owns its
files end-to-end (design → code → tests). LOC counts are from `wc -l`.

### Role 1 — Data Structures Lead (core DS layer)
- **Owns:** `include/data_structures/` — DynamicArray, LinkedList, HashMap, Queue, Stack, MinHeap, Hash — **891 LOC (~18%)**
- **Did:** designed the template container APIs used by everyone else; implemented growth/doubling, manual shifting, linked nodes + iterators, separate chaining with auto-rehash, sift-up/down with decrease-key; wrote the DS test blocks in `self_test.cpp`.
- **Why it matters:** every other component is built only on top of these — nothing touches the standard containers.

### Role 2 — Graph Engine & OOP Architect (domain + map)
- **Owns:** `Location.hpp`, `Connection.hpp`, `CampusMap.hpp/.cpp`, `MapGenerator.hpp/.cpp` — **624 LOC (~13%)**
- **Did:** designed the `Location → Building/Facility/Gate` hierarchy (encapsulation, inheritance, virtual polymorphism); built the adjacency-list graph with stable-id deletion; implemented the ASCII map renderer (Bresenham + char grid) and the campus reports; authored the demo campus seed data.

### Role 3 — Navigation & Algorithms Engineer
- **Owns:** `Navigation.hpp/.cpp`, `Route.hpp` — **280 LOC (~6% code, heavy on design/testing)**
- **Did:** implemented BFS (Queue), Dijkstra (MinHeap with decrease-key + tie-breaking for determinism), DFS (Stack), and both traversal orders; defined the `Route`/`HeapEntry` structures; proved optimality in tests (e.g. MainGate→Hostel-A = 290 m for all-pairs scenarios).

### Role 4 — Console UX & Quality Engineer
- **Owns:** `Menu.hpp/.cpp`, `main.cpp`, `tests/self_test.cpp` — **820 LOC (~17%)**
- **Did:** built the 10-option menu with id-or-name addressing, input validation, and pipe-friendly scripted runs; wrote the 91-assertion self-test suite covering data structures, graph operations, algorithms, and UI flows.

### Role 5 — Web Backend Engineer (server + API + security)
- **Owns:** `include/web/*`, `src/web/*`, `web_main.cpp`, `tests/web_test.cpp` — **1574 LOC (~32%)**
- **Did:** wrote the raw-socket HTTP server (parsing, cookies, static files, MIME, path safety); hand-rolled the JSON parser/writer; built the REST API layer over the engine; implemented session auth with role guards (401/403) using `ds::HashMap`; wrote the 41-check socket-level test suite.

### Role 6 — Frontend & Interaction Designer (browser app)
- **Owns:** `web/index.html`, `web/style.css`, `web/app.js` — **767 LOC (~15%)**
- **Did:** designed the single-page dark UI; built the canvas renderer (nodes, weighted edges, labels) with pan/zoom; wired search-as-you-type and the route panel; created the **admin map designer** (Place / Connect / Pan modes, drag-to-move with auto-save) and the role-aware interface (admin panel hidden from regular users).

### Share summary

| Role | Files | LOC | Share |
|---|---|---:|---:|
| 1. Data Structures Lead | 7 headers | 891 | ~18% |
| 2. Graph Engine & OOP | 6 files | 624 | ~13% |
| 3. Navigation & Algorithms | 3 files | 280 | ~6% (highest algorithmic complexity) |
| 4. Console UX & Quality | 4 files | 820 | ~17% |
| 5. Web Backend | 8 files | 1574 | ~32% |
| 6. Frontend Designer | 3 files | 767 | ~15% |
| **Total** | **31 files** | **≈4,950** | **100%** |

*Note: Role 3's line count is small but its design weight (correctness of
Dijkstra with decrease-key) is the highest in the project; Role 5's share
includes the largest test surface. Percentages reflect overall effort, not
just lines.*

---

## 7. Quick reference

```bash
build.bat            # build everything (Windows)
make tests           # console engine: 91 checks
make webtests        # web API: 41 checks
make serve           # start web server → http://localhost:18080
./mapgen.exe         # console app (10-option menu)
```

**Logins:** `admin/admin123` (full control + map designer) · `user/user123` (view/search/routes only)
