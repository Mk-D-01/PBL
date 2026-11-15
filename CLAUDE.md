# CLAUDE.md

## Overview
"MapGen Engine": campus locator/navigation in native C++17 (PBL course project, Team Hexagon).
Campus = graph (locations = nodes, walkways = weighted undirected edges). Two front ends over
one engine: a 10-option console menu (`mapgen`) and an HTTP/JSON server + vanilla-JS canvas UI
(`mapgen_web`).

**Hard course constraint:** all data structures are hand-written in `ds::` — no `std::vector`,
`std::map`, `std::priority_queue` in engine code. No third-party deps; no npm. Keep it that way.
(`std::string`, `std::ostringstream`, `std::function` etc. are used.)

## Commands (verified 2026-10-05, Windows 11, g++ from `D:\C\ucrt64\bin`)
`make` is NOT installed on this machine; the Makefile is untested here. Use `build.bat`.
```powershell
$env:PATH="D:\C\ucrt64\bin;$env:PATH"
cmd /c ".\build.bat"            # ~60 s, 0 warnings; builds all 4 exes into repo root
.\mapgen_tests.exe              # 107 checks, exit 0
.\mapgen_web_tests.exe          # 77 checks, exit 0 (binds port 18099 itself)
.\mapgen.exe                    # console menu
.\mapgen_web.exe --port 18080 --root web   # http://localhost:18080  (see caveat on --root)
```
- Plain `build.bat` from PowerShell fails ("not recognized") — use `.\build.bat` / `cmd /c ".\build.bat"`.
- Test exes return non-zero on failure; there is no test runner/CI. Tests are custom `require()`/`CHECK()` macros.
- Compile flags: `-std=c++17 -O2 -Wall -Wextra -Iinclude`; web targets add `-lws2_32`.
- Admin login: `admin/admin123`; read-only: `user/user123`. State is in-memory only; resets on restart.

## Architecture
```
web/ (index.html, app.js, style.css)  --HTTP/JSON-->  HttpServer -> ApiController -> CampusMap
Menu (console) ------------------------------------->                              -> Navigation (BFS/Dijkstra/DFS)
                                                                  all on ds:: containers
```
- `CampusMap` (graph engine): `locations_` = `DynamicArray<Location*>` indexed by id (removed slots become
  `nullptr`, ids never reused); `nameToId_` HashMap; `adjacency_` = `DynamicArray<LinkedList<Connection>>`
  (both half-edges stored); also owns a 62x20 ASCII grid (Bresenham) used only by console `renderMap()`.
- `Location` hierarchy: `Building` (floors), `Facility` (service text), `Gate` (open/closed); `kind()`/`describe()` virtual.
- `Navigation` is a `friend` of `CampusMap` and reads `adjacency_` directly. BFS=hops, Dijkstra (MinHeap + decreaseKey)=metres, DFS=reachability/traversal.
- `MapGenerator::generate` resets to 58x84 and seeds the floor plans in `frontWiew/` (Block A on top,
  Block B below, joined by "Link Passage"): 30 locations / 38 edges, A-Entrance = id 0. Floor-plan images: Block A = `frontWiew/er.png`,
  Block B = `frontWiew/Screenshot ...png`; `frontWiew/image.png` is the outdoor campus -> saved map `maps/outside.json`
  (9 locations / 15 assumed walkways; select via `/api/maps/select`).
- Web: `HttpServer` (raw Winsock/BSD sockets, single-threaded, one request/connection, loopback only),
  `Json` (hand-rolled parser/writer), `ApiController` (route table in `handle()`, cookie sessions in `ds::HashMap`).
- Endpoints: `/login /logout /api/whoami /api/map /api/search /api/route /api/info /api/traverse` (public),
  admin (401/403 guarded): `/api/generate`, `/api/maps` POST, `/api/maps/select|save|resize`, `/api/locations` POST/PUT, `/api/locations/{id}` DELETE, `/api/connections` POST/DELETE.

## Layout
```
include/            headers (campus:: domain, ds:: in data_structures/, web:: in web/)
src/                CampusMap, MapGenerator, Navigation, Menu, main, web_main; src/web/ = HttpServer, Json, ApiController
tests/              self_test.cpp (engine+console), web_test.cpp (real-socket API tests)
web/                static front end served by mapgen_web
docs/               PROJECT_DOCS.md (overview/roles), WEB_FRONTEND_PLAN.md (original plan; partly stale)
.freebuff/          previous agent's run notes (hardcodes D:\C\ucrt64\bin)
```
Committed binaries/artifacts: `mapgen*.exe` exist locally (gitignored). `GenMap [Autosaved].pptx` is a tracked
Office autosave (presentation, not code); its `~$` lock file shows as deleted in git status.

## Conventions observed
- Namespaces `campus`, `ds`, `web`; headers `#pragma once`; header-only templates in `ds/`.
- Types PascalCase, methods/vars camelCase, members trailing `_`, constants `kName` or `UPPER` (`GRID_WIDTH`).
- Errors: engine returns `bool` / `-1` / `nullptr` (no exceptions across the API); web returns
  `HttpResponse::error(status, msg)` with `{"error": ...}`; `try{stoi}catch(...){}` used for parsing input.
- Ids vs names: every user-facing lookup accepts id OR exact name (console and `/api/route`).
- State: engine holds all state in one `CampusMap`; frontend keeps a global `state` object and redraws the canvas wholesale after each mutation (`refr`eshMap()`).
- Comments are plentiful, explain "why", ASCII only in code.

## Known issues
Fixed (2026-10-19): `Navigation::valid()` slot-vs-count bug, idle-connection blocking (SO_RCVTIMEO),
`--root` ignored by the API. Coordinates are clamped to the *active map's* bounds (`minX..maxX`,
`minY..maxY`); maps are 20x10..200x100 and can be grown in place via `CampusMap::resize()` /
`POST /api/maps/resize` (refuses sizes that would cut off a location).
