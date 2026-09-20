#include "web/ApiController.hpp"

#include <cstdio>
#include <ctime>
#include <sstream>

namespace web {
namespace {

using campus::Connection;
using campus::Location;
using campus::LocationType;
using campus::Route;

std::string typeLetter(LocationType type) {
    switch (type) {
        case LocationType::Gate: return "G";
        case LocationType::Building: return "B";
        case LocationType::Facility: return "F";
    }
    return "B";
}

Json locationJson(const Location* loc) {
    Json obj = Json::makeObject();
    obj.members.put("id", Json::makeNumber(loc->id()));
    obj.members.put("name", Json::makeString(loc->name()));
    obj.members.put("x", Json::makeNumber(loc->x()));
    obj.members.put("y", Json::makeNumber(loc->y()));
    obj.members.put("type", Json::makeString(typeLetter(loc->kind())));
    obj.members.put("desc", Json::makeString(loc->describe()));
    return obj;
}

// Builds the connection list for one location; duplicates appear once (a < b).
Json connectionsJson(const campus::CampusMap& map) {
    Json arr = Json::makeArray();
    for (const Location* loc : map.allLocations()) {
        for (const Connection& conn : map.allConnectionsOf(loc->id())) {
            if (conn.toId <= loc->id()) continue;  // emit each undirected edge once
            Json edge = Json::makeObject();
            edge.members.put("from", Json::makeNumber(loc->id()));
            edge.members.put("to", Json::makeNumber(conn.toId));
            edge.members.put("d", Json::makeNumber(conn.distance));
            edge.members.put("label", Json::makeString(conn.label));
            arr.items.pushBack(edge);
        }
    }
    return arr;
}

// Path ids -> JSON array of names (easier to display in the browser).
Json pathJson(const campus::CampusMap& map, const Route& route) {
    Json arr = Json::makeArray();
    for (int id : route.path) {
        const Location* loc = map.findLocation(id);
        arr.items.pushBack(Json::makeString(loc != nullptr ? loc->name() : std::string("?")));
    }
    return arr;
}

std::string newToken(int& counter) {
    std::ostringstream ss;
    ss << "tok-" << counter++ << "-" << static_cast<int>(std::time(nullptr));
    return ss.str();
}

struct Account {
    const char* username;
    const char* password;
    Role role;
};

constexpr Account kAccounts[] = {
    {"admin", "admin123", Role::Admin},
    {"user", "user123", Role::User},
};

}  // namespace

ApiController::ApiController() {
    // Start with the default campus loaded so the map is never empty.
    campus::MapGenerator::generate(map_);
}

HttpResponse ApiController::handle(const HttpRequest& request) {
    const std::string& path = request.path;
    const std::string& method = request.method;

    // ---- auth ----
    if (path == "/login" && method == "POST") return handleLogin(request);
    if (path == "/logout" && method == "POST") return handleLogout(request);
    if (path == "/api/whoami" && method == "GET") return handleWhoami(request);

    // ---- user endpoints (any authenticated session) ----
    if (path == "/api/map" && method == "GET") return apiMap();
    if (path == "/api/search" && method == "GET") return apiSearch(request);
    if (path == "/api/route" && method == "GET") return apiRoute(request);
    if (path == "/api/info" && method == "GET") return apiInfo();
    if (path == "/api/traverse" && method == "GET") return apiTraverse(request);

    // ---- admin endpoints (must be authenticated, and PUT/POST must not
    // fall through into each other) ----
    if (path == "/api/generate" && method == "POST") return apiGenerate(request);
    if (path == "/api/locations" && (method == "POST" || method == "PUT")) {
        if (method == "PUT") return apiUpdateLocation(request, true);
        Json body;
        if (!Json::parse(request.body, body)) return HttpResponse::error(400, "invalid JSON body");
        // Body with an "id" field and no "type" is treated as an update for
        // client convenience; otherwise it creates a new location.
        if (body.get("id") != nullptr && body.get("type") == nullptr) {
            return apiUpdateLocation(request, false);
        }
        return apiAddLocation(request);
    }
    if (path.rfind("/api/locations/", 0) == 0 && method == "DELETE") {
        int id = 0;
        try {
            id = std::stoi(path.substr(15));
        } catch (...) {
            return HttpResponse::error(400, "invalid location id");
        }
        return apiDeleteLocation(request, id);
    }
    if (path == "/api/connections" && method == "POST") return apiAddConnection(request);
    if (path == "/api/connections" && method == "DELETE") return apiDeleteConnection(request);

    // ---- static front end (GET only, everything outside /api) ----
    if (method == "GET" && path.rfind("/api/", 0) != 0) {
        HttpResponse file;
        if (HttpServer::serveStatic("web", path, file)) return file;
        return HttpResponse::error(404, "file not found");
    }

    return HttpResponse::error(404, "unknown endpoint");
}

const WebSession* ApiController::sessionFor(const HttpRequest& request) const {
    std::string token = request.cookie("session");
    if (token.empty()) return nullptr;
    return sessions_.find(token);
}

const WebSession* ApiController::requireAdmin(const HttpRequest& request, HttpResponse& errorOut) const {
    const WebSession* session = sessionFor(request);
    if (session == nullptr) {
        errorOut = HttpResponse::error(401, "login required");
        return nullptr;
    }
    if (session->role != Role::Admin) {
        errorOut = HttpResponse::error(403, "admin role required");
        return nullptr;
    }
    return session;
}

HttpResponse ApiController::handleLogin(const HttpRequest& request) {
    Json body;
    if (!Json::parse(request.body, body)) {
        return HttpResponse::error(400, "invalid JSON body");
    }
    std::string username = body.getString("username");
    std::string password = body.getString("password");

    for (const Account& account : kAccounts) {
        if (username == account.username && password == account.password) {
            std::string token = newToken(nextToken_);
            WebSession session;
            session.username = username;
            session.role = account.role;
            sessions_.put(token, session);

            Json ok = Json::makeObject();
            ok.members.put("ok", Json::makeBool(true));
            ok.members.put("role", Json::makeString(account.role == Role::Admin ? "admin" : "user"));
            HttpResponse resp = HttpResponse::json(200, ok.dump());
            resp.setCookie = "session=" + token + "; Path=/; HttpOnly";
            return resp;
        }
    }
    return HttpResponse::error(401, "invalid credentials");
}

HttpResponse ApiController::handleLogout(const HttpRequest& request) {
    std::string token = request.cookie("session");
    if (!token.empty()) sessions_.remove(token);
    Json ok = Json::makeObject();
    ok.members.put("ok", Json::makeBool(true));
    HttpResponse resp = HttpResponse::json(200, ok.dump());
    resp.setCookie = "session=; Path=/; Max-Age=0";
    return resp;
}

HttpResponse ApiController::handleWhoami(const HttpRequest& request) {
    const WebSession* session = sessionFor(request);
    Json obj = Json::makeObject();
    if (session == nullptr) {
        obj.members.put("role", Json::makeString("guest"));
    } else {
        obj.members.put("username", Json::makeString(session->username));
        obj.members.put("role", Json::makeString(session->role == Role::Admin ? "admin" : "user"));
    }
    return HttpResponse::json(200, obj.dump());
}

HttpResponse ApiController::apiMap() {
    Json obj = Json::makeObject();
    Json locs = Json::makeArray();
    for (const Location* loc : map_.allLocations()) {
        locs.items.pushBack(locationJson(loc));
    }
    obj.members.put("locations", locs);
    obj.members.put("connections", connectionsJson(map_));
    return HttpResponse::json(200, obj.dump());
}

HttpResponse ApiController::apiSearch(const HttpRequest& request) {
    std::string query = request.queryParam("q");
    if (query.empty()) return HttpResponse::error(400, "missing query parameter 'q'");

    Json arr = Json::makeArray();
    // Exact name match first (short-circuit result).
    const Location* exact = map_.searchByName(query);
    if (exact != nullptr) arr.items.pushBack(locationJson(exact));
    // Then keyword matches (skip the exact duplicate).
    for (const Location* loc : map_.searchByKeyword(query)) {
        if (exact != nullptr && loc->id() == exact->id()) continue;
        arr.items.pushBack(locationJson(loc));
    }

    Json obj = Json::makeObject();
    obj.members.put("query", Json::makeString(query));
    obj.members.put("results", arr);
    return HttpResponse::json(200, obj.dump());
}

HttpResponse ApiController::apiRoute(const HttpRequest& request) {
    std::string from = request.queryParam("from");
    std::string to = request.queryParam("to");
    std::string metric = request.queryParam("metric");
    if (metric.empty()) metric = "distance";

    const Location* src = map_.searchByName(from);
    const Location* dst = map_.searchByName(to);
    // Fall back to numeric ids when names are not found.
    if (src == nullptr) {
        try {
            src = map_.findLocation(std::stoi(from));
        } catch (...) {}
    }
    if (dst == nullptr) {
        try {
            dst = map_.findLocation(std::stoi(to));
        } catch (...) {}
    }
    if (src == nullptr || dst == nullptr) {
        return HttpResponse::error(404, "source or destination not found");
    }

    campus::Navigation nav(map_);
    Route route = (metric == "hops") ? nav.shortestRouteByHops(src->id(), dst->id())
                                     : nav.shortestRouteByDistance(src->id(), dst->id());
    bool exists = nav.pathExists(src->id(), dst->id());

    Json obj = Json::makeObject();
    obj.members.put("reachable", Json::makeBool(route.reachable));
    obj.members.put("exists", Json::makeBool(exists));
    obj.members.put("metric", Json::makeString(metric == "hops" ? "hops" : "distance"));
    obj.members.put("totalDistance", Json::makeNumber(route.totalDistance));
    obj.members.put("hops", Json::makeNumber(route.hops));
    obj.members.put("path", pathJson(map_, route));
    return HttpResponse::json(200, obj.dump());
}

HttpResponse ApiController::apiInfo() {
    Json obj = Json::makeObject();
    obj.members.put("info", Json::makeString(map_.campusInfo()));
    obj.members.put("adjacency", Json::makeString(map_.adjacencyReport()));
    return HttpResponse::json(200, obj.dump());
}

HttpResponse ApiController::apiTraverse(const HttpRequest& request) {
    std::string from = request.queryParam("from");
    std::string order = request.queryParam("order");
    if (order.empty()) order = "bfs";

    const Location* src = map_.searchByName(from);
    if (src == nullptr) {
        try {
            src = map_.findLocation(std::stoi(from));
        } catch (...) {}
    }
    if (src == nullptr) return HttpResponse::error(404, "source not found");

    campus::Navigation nav(map_);
    ds::DynamicArray<int> visit = (order == "dfs") ? nav.traverseDFS(src->id()) : nav.traverseBFS(src->id());

    Json arr = Json::makeArray();
    for (int id : visit) {
        const Location* loc = map_.findLocation(id);
        if (loc != nullptr) arr.items.pushBack(Json::makeString(loc->name()));
    }
    Json obj = Json::makeObject();
    obj.members.put("order", Json::makeString(order == "dfs" ? "dfs" : "bfs"));
    obj.members.put("visit", arr);
    return HttpResponse::json(200, obj.dump());
}

HttpResponse ApiController::apiGenerate(const HttpRequest& request) {
    HttpResponse denied;
    if (requireAdmin(request, denied) == nullptr) return denied;
    campus::MapGenerator::generate(map_);
    Json ok = Json::makeObject();
    ok.members.put("ok", Json::makeBool(true));
    ok.members.put("locations", Json::makeNumber(map_.locationCount()));
    ok.members.put("connections", Json::makeNumber(map_.edgeCount()));
    return HttpResponse::json(200, ok.dump());
}

HttpResponse ApiController::apiAddLocation(const HttpRequest& request) {
    HttpResponse denied;
    if (requireAdmin(request, denied) == nullptr) return denied;
    Json body;
    if (!Json::parse(request.body, body)) return HttpResponse::error(400, "invalid JSON body");

    std::string name = body.getString("name");
    std::string type = body.getString("type", "F");
    int x = body.getInt("x", -1);
    int y = body.getInt("y", -1);
    std::string detail = body.getString("detail");

    if (name.empty() || x < 0 || y < 0) {
        return HttpResponse::error(400, "name, x and y are required");
    }
    LocationType kind = LocationType::Facility;
    if (type == "B" || type == "Building") {
        kind = LocationType::Building;
    } else if (type == "G" || type == "Gate") {
        kind = LocationType::Gate;
    }

    int id = map_.addLocation(name, kind, x, y, detail);
    if (id < 0) return HttpResponse::error(400, "could not add location (duplicate name?)");

    Json ok = Json::makeObject();
    ok.members.put("ok", Json::makeBool(true));
    ok.members.put("id", Json::makeNumber(id));
    return HttpResponse::json(201, ok.dump());
}

HttpResponse ApiController::apiUpdateLocation(const HttpRequest& request, bool isPut) {
    (void)isPut;
    HttpResponse denied;
    if (requireAdmin(request, denied) == nullptr) return denied;
    Json body;
    if (!Json::parse(request.body, body)) return HttpResponse::error(400, "invalid JSON body");
    int id = body.getInt("id", -1);
    if (id < 0) return HttpResponse::error(400, "location id is required");

    bool okAny = false;
    std::string newName = body.getString("name");
    if (!newName.empty()) {
        okAny = map_.updateLocation(id, newName) || okAny;
    }
    if (body.get("x") != nullptr && body.get("y") != nullptr) {
        okAny = map_.updateLocation(id, body.getInt("x"), body.getInt("y")) || okAny;
    }
    if (!okAny) return HttpResponse::error(400, "nothing updated (bad id or name taken)");

    Json ok = Json::makeObject();
    ok.members.put("ok", Json::makeBool(true));
    return HttpResponse::json(200, ok.dump());
}

HttpResponse ApiController::apiDeleteLocation(const HttpRequest& request, int idFromPath) {
    (void)idFromPath;
    HttpResponse denied;
    if (requireAdmin(request, denied) == nullptr) return denied;
    if (!map_.removeLocation(idFromPath)) {
        return HttpResponse::error(404, "location not found");
    }
    Json ok = Json::makeObject();
    ok.members.put("ok", Json::makeBool(true));
    return HttpResponse::json(200, ok.dump());
}

HttpResponse ApiController::apiAddConnection(const HttpRequest& request) {
    HttpResponse denied;
    if (requireAdmin(request, denied) == nullptr) return denied;
    Json body;
    if (!Json::parse(request.body, body)) return HttpResponse::error(400, "invalid JSON body");
    int fromId = body.getInt("from", -1);
    int toId = body.getInt("to", -1);
    int distance = body.getInt("d", 1);
    std::string label = body.getString("label");
    if (fromId < 0 || toId < 0) return HttpResponse::error(400, "from and to are required");

    if (!map_.connect(fromId, toId, distance, label)) {
        return HttpResponse::error(400, "could not connect (bad ids or duplicate edge)");
    }
    Json ok = Json::makeObject();
    ok.members.put("ok", Json::makeBool(true));
    return HttpResponse::json(201, ok.dump());
}

HttpResponse ApiController::apiDeleteConnection(const HttpRequest& request) {
    HttpResponse denied;
    if (requireAdmin(request, denied) == nullptr) return denied;
    std::string fromStr = request.queryParam("from");
    std::string toStr = request.queryParam("to");
    if (fromStr.empty() || toStr.empty()) return HttpResponse::error(400, "from and to are required");

    int fromId = -1, toId = -1;
    const Location* src = map_.searchByName(fromStr);
    if (src != nullptr) {
        fromId = src->id();
    } else {
        try {
            fromId = std::stoi(fromStr);
        } catch (...) {}
    }
    const Location* dst = map_.searchByName(toStr);
    if (dst != nullptr) {
        toId = dst->id();
    } else {
        try {
            toId = std::stoi(toStr);
        } catch (...) {}
    }
    if (!map_.disconnect(fromId, toId)) return HttpResponse::error(404, "connection not found");

    Json ok = Json::makeObject();
    ok.members.put("ok", Json::makeBool(true));
    return HttpResponse::json(200, ok.dump());
}

}  // namespace web
