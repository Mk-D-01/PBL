#pragma once

#include <memory>
#include <string>

#include "CampusMap.hpp"
#include "MapGenerator.hpp"
#include "Navigation.hpp"
#include "web/HttpServer.hpp"
#include "web/Json.hpp"
#include "web/MapStore.hpp"
#include "data_structures/HashMap.hpp"

namespace web {

enum class Role { User, Admin };

// WebSession: a logged-in browser session (token -> role), stored in the
// project's custom ds::HashMap — no std::map anywhere in the web layer.
struct WebSession {
    std::string username;
    Role role = Role::User;
};

// ApiController: wires HTTP endpoints to the CampusMap engine.
// All handlers run sequentially inside the single-threaded server loop, so no
// synchronization is needed.
class ApiController {
public:
    // `mapsDir` is where saved maps live (one <name>.json each).
    explicit ApiController(std::string mapsDir = "maps");

    // Main dispatch entry passed to HttpServer.
    HttpResponse handle(const HttpRequest& request);

private:
    // ---- auth ----
    HttpResponse handleLogin(const HttpRequest& request);
    HttpResponse handleLogout(const HttpRequest& request);
    HttpResponse handleWhoami(const HttpRequest& request);
    // Returns the session for the request's cookie, or nullptr.
    const WebSession* sessionFor(const HttpRequest& request) const;

    // ---- user endpoints (read-only) ----
    HttpResponse apiMap();
    HttpResponse apiSearch(const HttpRequest& request);
    HttpResponse apiRoute(const HttpRequest& request);
    HttpResponse apiInfo();
    HttpResponse apiTraverse(const HttpRequest& request);

    // ---- admin endpoints (mutating) ----
    HttpResponse apiGenerate(const HttpRequest& request);
    HttpResponse apiAddLocation(const HttpRequest& request);
    HttpResponse apiUpdateLocation(const HttpRequest& request, bool isPut);
    HttpResponse apiDeleteLocation(const HttpRequest& request, int idFromPath);
    HttpResponse apiAddConnection(const HttpRequest& request);
    HttpResponse apiDeleteConnection(const HttpRequest& request);

    // ---- maps (list is public; create/select/save are admin-only) ----
    HttpResponse apiMaps();
    HttpResponse apiCreateMap(const HttpRequest& request);
    HttpResponse apiSelectMap(const HttpRequest& request);
    HttpResponse apiSaveMap(const HttpRequest& request);

    // ---- helpers ----
    // Loads a saved map into a fresh CampusMap and, only if that fully succeeds, makes
    // it the active one (activeName_ = name, dirty_ = false). On failure nothing changes.
    bool activateSavedMap(const std::string& name, std::string& error);
    // 401/403 JSON error when the caller is not an admin; nullptr otherwise
    // (and the session is returned through `out`).
    const WebSession* requireAdmin(const HttpRequest& request, HttpResponse& errorOut) const;

    // The one map every client sees (global). A pointer so a freshly loaded map can
    // replace it atomically: CampusMap itself is non-copyable.
    std::unique_ptr<campus::CampusMap> map_;
    MapStore store_;
    std::string activeName_ = "default";  // name the active map saves under
    bool dirty_ = false;                  // edited since the last load/save?
    campus::MapGenerator generator_;
    ds::HashMap<std::string, WebSession> sessions_;  // token -> session
    int nextToken_ = 1;
};

}  // namespace web
