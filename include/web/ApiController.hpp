#pragma once

#include <string>

#include "CampusMap.hpp"
#include "MapGenerator.hpp"
#include "Navigation.hpp"
#include "web/HttpServer.hpp"
#include "web/Json.hpp"
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
    ApiController();

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

    // ---- helpers ----
    // 401/403 JSON error when the caller is not an admin; nullptr otherwise
    // (and the session is returned through `out`).
    const WebSession* requireAdmin(const HttpRequest& request, HttpResponse& errorOut) const;

    campus::CampusMap map_;
    campus::MapGenerator generator_;
    ds::HashMap<std::string, WebSession> sessions_;  // token -> session
    int nextToken_ = 1;
};

}  // namespace web
