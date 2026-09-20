# MapGen Engine: Campus Locator and Navigation

A native C++ console application that models a college campus as a graph of
locations and walkways, and provides location management, search, map
rendering, and route finding — implementing the proposal "MapGen Engine:
Campus Locator and Navigation" (Team Hexagon).

Built for the course requirement: **all data structures are hand-implemented
from scratch** — the engine uses no `std::vector`, `std::map`, or
`std::priority_queue`.

## Build & Run

Requires g++ (MinGW-w64 / MSYS2 on Windows) or any C++17 compiler.

### Console app

```bash
# Option 1: make
make            # builds ./mapgen
make tests      # builds and runs the self-test suite (91 checks)
make run        # build + start the interactive menu

# Option 2: Windows batch script
build.bat       # builds mapgen.exe, mapgen_web.exe and both test binaries

# Option 3: direct
g++ -std=c++17 -O2 -Wall -Wextra -Iinclude src/main.cpp \
    src/CampusMap.cpp src/MapGenerator.cpp src/Navigation.cpp src/Menu.cpp -o mapgen
./mapgen
```

### Web front end

```bash
make web        # builds ./mapgen_web (HTTP server + browser UI)
make serve      # build + start, then open http://localhost:18080
# or: ./mapgen_web --port 18080 --root web
```

Open **http://localhost:18080** in a browser. Accounts:

| Account | Password | Can do |
|---------|----------|--------|
| `user`  | `user123` | View map, search locations, find routes |
| `admin` | `admin123` | Everything above **plus** generate the default campus, add/update/remove locations, add/remove connections |

The browser UI is a single page (`web/index.html`, `web/style.css`, `web/app.js`):
a canvas map renderer with pan (drag) and zoom (wheel), search-as-you-type,
a route panel (BFS "fewest stops" / Dijkstra "shortest distance"), and an
admin panel that is hidden unless an admin session is active.

The server is implemented natively in C++ (`include/web/`, `src/web/`) with a
hand-rolled JSON parser/writer and the project's own `ds::HashMap` for
sessions — no external libraries, keeping the custom-data-structures
requirement intact. It listens on `127.0.0.1` only and handles one request
per connection, single-threaded (fine for a demo; documented limitation).

```bash
make webtests   # builds and runs the web API suite (41 checks over real sockets)
```

## The 10-Option Menu

```
 1. Generate Campus Map      6. Update Location
 2. Display Map              7. Add Connection
 3. Add Location             8. Find Route
 4. Remove Location          9. Display Campus Information
 5. Search Location         10. Exit
```

Locations can be referenced by **id or exact name** in every prompt.
Search matches partial, case-insensitive keywords.

## Project Structure

```
include/
  data_structures/        <- hand-implemented data structures (the core DS layer)
    DynamicArray.hpp        growable array (doubling strategy, manual shifting)
    LinkedList.hpp          doubly linked list with iterators
    Stack.hpp               LIFO (singly linked nodes)
    Queue.hpp               FIFO (singly linked nodes)
    HashMap.hpp             separate-chaining hash table (FNV-1a/Murmur hash,
                            load factor 0.75, automatic rehash)
    MinHeap.hpp             binary min-heap with decrease-key (priority queue)
    Hash.hpp                FNV-1a (strings) + Murmur3 finalizer (integers)
  Location.hpp            abstract Location + Building / Facility / Gate
  Connection.hpp          undirected weighted edge
  Route.hpp               navigation result + Dijkstra heap entry
  CampusMap.hpp           the graph engine (declaration)
  MapGenerator.hpp        predefined campus seed data (declaration)
  Navigation.hpp          BFS / Dijkstra / DFS (declaration)
  Menu.hpp                console UI (declaration)
src/                      implementations of the above
include/web/              HTTP layer: HttpServer, hand-rolled JSON, ApiController
src/web/                  implementations of the above
web/                      browser front end (index.html, style.css, app.js)
tests/self_test.cpp       91 assertions: data structures, graph ops, algorithms, UI
tests/web_test.cpp        41 assertions: HTTP API, auth/roles, CRUD round-trips
```

## Data Structures Used (all hand-written)

| Structure        | Where it is used                                                        |
|------------------|-------------------------------------------------------------------------|
| DynamicArray     | node table, adjacency table, 2-D map grid, paths, traversal orders       |
| LinkedList       | adjacency buckets (each location's connection list), hash-map chaining   |
| HashMap          | name → id index; visited / parent / distance sets in every algorithm     |
| Queue            | BFS frontier (fewest-stops routing and BFS traversal)                    |
| Stack            | DFS (path existence check and DFS traversal)                             |
| MinHeap          | Dijkstra's priority queue with decrease-key                              |

## OOP Concepts Demonstrated

- **Encapsulation** — `Location`, `CampusMap`, `Navigation` hide all data
  behind methods; no public data members anywhere.
- **Inheritance** — `Building`, `Facility`, `Gate` derive from `Location`.
- **Polymorphism** — `kind()` and `describe()` are virtual; the map, reports
  and search work through `Location*` without knowing concrete subtypes.
- **Abstraction** — `CampusMap` exposes add/remove/update/search/connect;
  the grid rendering and adjacency internals are private.
- **Constructors / destructors** — deep copy & move semantics on every custom
  container; `CampusMap` owns and frees its polymorphic `Location` objects.

## Algorithms

| Algorithm                 | Purpose                                | Data structures           |
|---------------------------|----------------------------------------|---------------------------|
| BFS                       | route with fewest stops; level traversal | Queue + HashMap          |
| Dijkstra                  | shortest route by distance (weights)   | MinHeap + HashMap         |
| DFS (iterative)           | connectivity check ("is there a path") | Stack + HashMap           |
| Bresenham line drawing    | renders walkways on the ASCII map grid | DynamicArray grid         |

## Generated Demo Campus

`MapGenerator::generate` seeds 12 locations — MainGate, EastGate, CSE/ECE/ME/
Admin blocks, Library, Cafeteria, Hostel-A, MedicalCtr, SportsField, Parking —
and 20 labelled walkway connections. Example (MainGate → Hostel-A):

```
Route [fewest stops (BFS)]:
  1. MainGate
  2. Admin-Block
  3. Cafeteria
  4. Hostel-A
  Stops: 3, Total distance: 290 m
```

Dijkstra returns the same route here (80 + 120 + 90 m); on other queries it
prefers shorter total distance even when that means more stops.

## Notes & Assumptions

- The map is a simplified logical layout on a text grid, not geographically
  accurate (as stated in the proposal).
- Distances are edge weights in metres; "hops" count edges.
- Removing a location keeps remaining ids stable and automatically drops all
  of its connections.
- Duplicate connections, self-loops and duplicate names are rejected.
