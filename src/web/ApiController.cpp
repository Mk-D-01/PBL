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

// The map that exists even before anything is saved: the generated campus.
constexpr const char* kBuiltinName = "default";

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

ApiController::ApiController(std::string mapsDir) : store_(std::move(mapsDir)) {
    map_ = std::make_unique<campus::CampusMap>();
    // A saved "default" map wins, so edits survive a restart; otherwise seed the
    // built-in campus so the map is never empty.
    std::string error;
    if (store_.exists(activeName_) && activateSavedMap(activeName_, error)) return;
    if (!error.empty()) {
        std::printf("[maps] could not load '%s': %s - using the built-in campus\n",
                    activeName_.c_str(), error.c_str());
        std::fflush(stdout);
    }
    campus::MapGenerator::generate(*map_);
}

bool ApiController::activateSavedMap(const std::string& name, std::string& error) {
    auto fresh = std::make_unique<campus::CampusMap>();
    if (!store_.load(name, *fresh, error)) return false;
    map_ = std::move(fresh);
    activeName_ = name;
    dirty_ = false;
    return true;
}

const campus::CampusMap* ApiController::mapForView(const HttpRequest& request,
                                                   std::unique_ptr<campus::CampusMap>& holder,
                                                   HttpResponse& error) const {
    const std::string name = request.queryParam("map");
    if (name.empty() || name == activeName_) return map_.get();  // live map, incl. unsaved edits

    if (!MapStore::validName(name)) {
        error = HttpResponse::error(400, "invalid map name");
        return nullptr;
    }
    holder = std::make_unique<campus::CampusMap>();
    if (store_.exists(name)) {
        std::string loadError;
        if (!store_.load(name, *holder, loadError)) {
            error = HttpResponse::error(400, loadError);
            return nullptr;
        }
    } else if (name == kBuiltinName) {
        campus::MapGenerator::generate(*holder);
    } else {
        error = HttpResponse::error(404, "map not found");
        return nullptr;
    }
    return holder.get();
}

HttpResponse ApiController::handle(const HttpRequest& request) {
    const std::string& path = request.path;
    const std::string& method = request.method;

    // ---- auth ----
    if (path == "/login" && method == "POST") return handleLogin(request);
    if (path == "/logout" && method == "POST") return handleLogout(request);
    if (path == "/api/whoami" && method == "GET") return handleWhoami(request);

    // ---- user endpoints (any authenticated session) ----
    if (path == "/api/map" && method == "GET") return apiMap(request);
    if (path == "/api/search" && method == "GET") return apiSearch(request);
    if (path == "/api/route" && method == "GET") return apiRoute(request);
    if (path == "/api/info" && method == "GET") return apiInfo(request);
    if (path == "/api/traverse" && method == "GET") return apiTraverse(request);
    if (path == "/api/maps" && method == "GET") return apiMaps();

    // ---- map management (admin only, checked inside) ----
    if (path == "/api/maps" && method == "POST") return apiCreateMap(request);
    if (path == "/api/maps/select" && method == "POST") return apiSelectMap(request);
    if (path == "/api/maps/save" && method == "POST") return apiSaveMap(request);

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
            std::printf("[login] OK     user='%s' role=%s\n", username.c_str(),
                        account.role == Role::Admin ? "admin" : "user");
            std::fflush(stdout);
            return resp;
        }
    }
    // Log the attempt but never the password itself (only its length).
    std::printf("[login] FAILED user='%s' password_length=%zu\n", username.c_str(), password.size());
    std::fflush(stdout);
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

HttpResponse ApiController::apiMap(const HttpRequest& request) {
    std::unique_ptr<campus::CampusMap> holder;
    HttpResponse error;
    const campus::CampusMap* view = mapForView(request, holder, error);
    if (view == nullptr) return error;
    const bool isActive = (view == map_.get());

    Json obj = Json::makeObject();
    Json locs = Json::makeArray();
    for (const Location* loc : view->allLocations()) {
        locs.items.pushBack(locationJson(loc));
    }
    obj.members.put("locations", locs);
    obj.members.put("connections", connectionsJson(*view));
    obj.members.put("name", Json::makeString(isActive ? activeName_ : request.queryParam("map")));
    obj.members.put("width", Json::makeNumber(view->width()));
    obj.members.put("height", Json::makeNumber(view->height()));
    // The area locations may occupy; coordinates outside it are clamped by the engine.
    Json bounds = Json::makeObject();
    bounds.members.put("minX", Json::makeNumber(view->minX()));
    bounds.members.put("maxX", Json::makeNumber(view->maxX()));
    bounds.members.put("minY", Json::makeNumber(view->minY()));
    bounds.members.put("maxY", Json::makeNumber(view->maxY()));
    obj.members.put("bounds", bounds);
    obj.members.put("dirty", Json::makeBool(isActive && dirty_));
    return HttpResponse::json(200, obj.dump());
}

HttpResponse ApiController::apiSearch(const HttpRequest& request) {
    std::string query = request.queryParam("q");
    if (query.empty()) return HttpResponse::error(400, "missing query parameter 'q'");
    std::unique_ptr<campus::CampusMap> holder;
    HttpResponse error;
    const campus::CampusMap* view = mapForView(request, holder, error);
    if (view == nullptr) return error;

    Json arr = Json::makeArray();
    // Exact name match first (short-circuit result).
    const Location* exact = view->searchByName(query);
    if (exact != nullptr) arr.items.pushBack(locationJson(exact));
    // Then keyword matches (skip the exact duplicate).
    for (const Location* loc : view->searchByKeyword(query)) {
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
    std::unique_ptr<campus::CampusMap> holder;
    HttpResponse error;
    const campus::CampusMap* view = mapForView(request, holder, error);
    if (view == nullptr) return error;

    const Location* src = view->searchByName(from);
    const Location* dst = view->searchByName(to);
    // Fall back to numeric ids when names are not found.
    if (src == nullptr) {
        try {
            src = view->findLocation(std::stoi(from));
        } catch (...) {}
    }
    if (dst == nullptr) {
        try {
            dst = view->findLocation(std::stoi(to));
        } catch (...) {}
    }
    if (src == nullptr || dst == nullptr) {
        return HttpResponse::error(404, "source or destination not found");
    }

    campus::Navigation nav(*view);
    Route route = (metric == "hops") ? nav.shortestRouteByHops(src->id(), dst->id())
                                     : nav.shortestRouteByDistance(src->id(), dst->id());
    bool exists = nav.pathExists(src->id(), dst->id());

    Json obj = Json::makeObject();
    obj.members.put("reachable", Json::makeBool(route.reachable));
    obj.members.put("exists", Json::makeBool(exists));
    obj.members.put("metric", Json::makeString(metric == "hops" ? "hops" : "distance"));
    obj.members.put("totalDistance", Json::makeNumber(route.totalDistance));
    obj.members.put("hops", Json::makeNumber(route.hops));
    obj.members.put("path", pathJson(*view, route));
    return HttpResponse::json(200, obj.dump());
}

HttpResponse ApiController::apiInfo(const HttpRequest& request) {
    std::unique_ptr<campus::CampusMap> holder;
    HttpResponse error;
    const campus::CampusMap* view = mapForView(request, holder, error);
    if (view == nullptr) return error;

    Json obj = Json::makeObject();
    obj.members.put("info", Json::makeString(view->campusInfo()));
    obj.members.put("adjacency", Json::makeString(view->adjacencyReport()));
    return HttpResponse::json(200, obj.dump());
}

HttpResponse ApiController::apiTraverse(const HttpRequest& request) {
    std::string from = request.queryParam("from");
    std::string order = request.queryParam("order");
    if (order.empty()) order = "bfs";
    std::unique_ptr<campus::CampusMap> holder;
    HttpResponse error;
    const campus::CampusMap* view = mapForView(request, holder, error);
    if (view == nullptr) return error;

    const Location* src = view->searchByName(from);
    if (src == nullptr) {
        try {
            src = view->findLocation(std::stoi(from));
        } catch (...) {}
    }
    if (src == nullptr) return HttpResponse::error(404, "source not found");

    campus::Navigation nav(*view);
    ds::DynamicArray<int> visit = (order == "dfs") ? nav.traverseDFS(src->id()) : nav.traverseBFS(src->id());

    Json arr = Json::makeArray();
    for (int id : visit) {
        const Location* loc = view->findLocation(id);
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
    campus::MapGenerator::generate(*map_);
    dirty_ = true;
    Json ok = Json::makeObject();
    ok.members.put("ok", Json::makeBool(true));
    ok.members.put("locations", Json::makeNumber(map_->locationCount()));
    ok.members.put("connections", Json::makeNumber(map_->edgeCount()));
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

    int id = map_->addLocation(name, kind, x, y, detail);
    if (id < 0) return HttpResponse::error(400, "could not add location (duplicate name?)");
    dirty_ = true;

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
        okAny = map_->updateLocation(id, newName) || okAny;
    }
    if (body.get("x") != nullptr && body.get("y") != nullptr) {
        okAny = map_->updateLocation(id, body.getInt("x"), body.getInt("y")) || okAny;
    }
    if (!okAny) return HttpResponse::error(400, "nothing updated (bad id or name taken)");
    dirty_ = true;

    Json ok = Json::makeObject();
    ok.members.put("ok", Json::makeBool(true));
    return HttpResponse::json(200, ok.dump());
}

HttpResponse ApiController::apiDeleteLocation(const HttpRequest& request, int idFromPath) {
    (void)idFromPath;
    HttpResponse denied;
    if (requireAdmin(request, denied) == nullptr) return denied;
    if (!map_->removeLocation(idFromPath)) {
        return HttpResponse::error(404, "location not found");
    }
    dirty_ = true;
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

    if (!map_->connect(fromId, toId, distance, label)) {
        return HttpResponse::error(400, "could not connect (bad ids or duplicate edge)");
    }
    dirty_ = true;
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
    const Location* src = map_->searchByName(fromStr);
    if (src != nullptr) {
        fromId = src->id();
    } else {
        try {
            fromId = std::stoi(fromStr);
        } catch (...) {}
    }
    const Location* dst = map_->searchByName(toStr);
    if (dst != nullptr) {
        toId = dst->id();
    } else {
        try {
            toId = std::stoi(toStr);
        } catch (...) {}
    }
    if (!map_->disconnect(fromId, toId)) return HttpResponse::error(404, "connection not found");
    dirty_ = true;

    Json ok = Json::makeObject();
    ok.members.put("ok", Json::makeBool(true));
    return HttpResponse::json(200, ok.dump());
}

// ------------------------------------------------------------------- maps

HttpResponse ApiController::apiMaps() {
    ds::DynamicArray<std::string> names = store_.list();
    // Always offer the built-in campus and the active map, even when neither is saved yet.
    for (const std::string& extra : {std::string(kBuiltinName), activeName_}) {
        bool listed = false;
        for (const std::string& name : names) listed = listed || name == extra;
        if (listed) continue;
        std::size_t pos = 0;
        while (pos < names.size() && names[pos] < extra) ++pos;
        names.insertAt(pos, extra);
    }

    Json arr = Json::makeArray();
    for (const std::string& name : names) {
        Json entry = Json::makeObject();
        entry.members.put("name", Json::makeString(name));
        entry.members.put("saved", Json::makeBool(store_.exists(name)));
        arr.items.pushBack(entry);
    }
    Json obj = Json::makeObject();
    obj.members.put("active", Json::makeString(activeName_));
    obj.members.put("dirty", Json::makeBool(dirty_));
    obj.members.put("maps", arr);
    return HttpResponse::json(200, obj.dump());
}

HttpResponse ApiController::apiCreateMap(const HttpRequest& request) {
    HttpResponse denied;
    if (requireAdmin(request, denied) == nullptr) return denied;
    Json body;
    if (!Json::parse(request.body, body)) return HttpResponse::error(400, "invalid JSON body");

    const std::string name = body.getString("name");
    const int width = body.getInt("width", campus::CampusMap::GRID_WIDTH);
    const int height = body.getInt("height", campus::CampusMap::GRID_HEIGHT);
    if (!MapStore::validName(name)) {
        return HttpResponse::error(400, "invalid map name (letters, digits, '_' or '-', up to 40 characters)");
    }
    if (name == activeName_ || name == kBuiltinName || store_.exists(name)) {
        return HttpResponse::error(400, "a map with that name already exists");
    }

    auto fresh = std::make_unique<campus::CampusMap>();
    if (!fresh->reset(width, height)) {
        return HttpResponse::error(
            400, "size must be " + std::to_string(campus::CampusMap::MIN_WIDTH) + "-" +
                     std::to_string(campus::CampusMap::MAX_WIDTH) + " wide and " +
                     std::to_string(campus::CampusMap::MIN_HEIGHT) + "-" +
                     std::to_string(campus::CampusMap::MAX_HEIGHT) + " high");
    }
    // Save the empty map right away so it shows up in the dropdown and survives a restart.
    std::string error;
    if (!store_.save(name, *fresh, error)) return HttpResponse::error(500, error);

    map_ = std::move(fresh);
    activeName_ = name;
    dirty_ = false;
    std::printf("[maps] created '%s' (%dx%d)\n", name.c_str(), width, height);
    std::fflush(stdout);

    Json ok = Json::makeObject();
    ok.members.put("ok", Json::makeBool(true));
    ok.members.put("name", Json::makeString(name));
    return HttpResponse::json(201, ok.dump());
}

HttpResponse ApiController::apiSelectMap(const HttpRequest& request) {
    HttpResponse denied;
    if (requireAdmin(request, denied) == nullptr) return denied;
    Json body;
    if (!Json::parse(request.body, body)) return HttpResponse::error(400, "invalid JSON body");

    const std::string name = body.getString("name");
    if (!MapStore::validName(name)) return HttpResponse::error(400, "invalid map name");
    // Re-selecting the active map keeps the in-memory edits instead of reloading from disk.
    if (name != activeName_) {
        if (store_.exists(name)) {
            std::string error;
            if (!activateSavedMap(name, error)) return HttpResponse::error(400, error);
        } else if (name == kBuiltinName) {
            // Never saved (or file removed): rebuild the built-in campus.
            auto fresh = std::make_unique<campus::CampusMap>();
            campus::MapGenerator::generate(*fresh);
            map_ = std::move(fresh);
            activeName_ = name;
            dirty_ = false;
        } else {
            return HttpResponse::error(404, "map not found");
        }
        std::printf("[maps] switched to '%s'\n", name.c_str());
        std::fflush(stdout);
    }

    Json ok = Json::makeObject();
    ok.members.put("ok", Json::makeBool(true));
    ok.members.put("name", Json::makeString(activeName_));
    return HttpResponse::json(200, ok.dump());
}

HttpResponse ApiController::apiSaveMap(const HttpRequest& request) {
    HttpResponse denied;
    if (requireAdmin(request, denied) == nullptr) return denied;

    // Optional {"name": "..."} saves under a new name ("Save As"); otherwise the active name.
    std::string target = activeName_;
    if (!request.body.empty()) {
        Json body;
        if (!Json::parse(request.body, body)) return HttpResponse::error(400, "invalid JSON body");
        const std::string requested = body.getString("name");
        if (!requested.empty()) target = requested;
    }
    if (!MapStore::validName(target)) {
        return HttpResponse::error(400, "invalid map name (letters, digits, '_' or '-', up to 40 characters)");
    }
    // Never silently overwrite a different saved map.
    if (target != activeName_ && store_.exists(target)) {
        return HttpResponse::error(400, "a map with that name already exists");
    }

    std::string error;
    if (!store_.save(target, *map_, error)) return HttpResponse::error(500, error);
    activeName_ = target;
    dirty_ = false;
    std::printf("[maps] saved '%s' (%d locations, %d connections)\n", target.c_str(),
                map_->locationCount(), map_->edgeCount());
    std::fflush(stdout);

    Json ok = Json::makeObject();
    ok.members.put("ok", Json::makeBool(true));
    ok.members.put("name", Json::makeString(target));
    return HttpResponse::json(200, ok.dump());
}

}  // namespace web
