# Run Doc — MapGen Engine web front end

Native C++ project (no npm/node). One binary serves both the JSON API and the
static front end in `web/`.

## Reproduce the artifacts

Compile `mapgen_web.exe` with g++ (MinGW-w64 / MSYS2 ucrt64 — on this machine
g++ lives in `D:\C\ucrt64\bin`, so prepend it to PATH in bash):

```bash
export PATH="/d/C/ucrt64/bin:$PATH"
g++ -std=c++17 -O2 -Wall -Wextra -Iinclude \
    src/web_main.cpp src/web/HttpServer.cpp src/web/Json.cpp src/web/ApiController.cpp \
    src/CampusMap.cpp src/MapGenerator.cpp src/Navigation.cpp src/Menu.cpp \
    -o mapgen_web.exe -lws2_32
```

There are no env files to copy and no dependency install step. The engine
boots with the default 12-location demo campus seeded in memory; admin edits
are in-memory only and reset on restart.

Other binaries (same recipe, different sources):
- `mapgen.exe` — console app: swap `src/web_main.cpp src/web/*` for `src/main.cpp` (no `-lws2_32`).
- Test suites: `tests/web_test.cpp` + web sources (with `-lws2_32`), and `tests/self_test.cpp` + engine sources.

## Run the server

```bash
./mapgen_web.exe --port 18080 --root web
```

- Binds to `127.0.0.1` only; `--port` (default 18080) and `--root` (default `web`) are optional.
- URL: **http://localhost:18080** — one page (`web/index.html` + `app.js` + `style.css`).
- Accounts: `admin/admin123` (full CRUD), `user/user123` (read-only search/map/routes).
- Start it detached (bash background `(... &)` works) and confirm with
  `curl -s -o /dev/null -w "%{http_code}\n" http://localhost:18080/` → expect `200`.
