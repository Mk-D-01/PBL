#include "web/MapStore.hpp"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "web/Json.hpp"

namespace web {
namespace {

namespace fs = std::filesystem;
using campus::Building;
using campus::Connection;
using campus::Facility;
using campus::Gate;
using campus::Location;
using campus::LocationType;

constexpr int kMaxLocationId = 100000;  // sanity cap so a corrupt file cannot allocate gigabytes

std::string upper(std::string text) {
    for (char& ch : text) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    return text;
}

bool isReservedDeviceName(const std::string& name) {
    const std::string n = upper(name);
    if (n == "CON" || n == "PRN" || n == "AUX" || n == "NUL") return true;
    if (n.size() == 4 && (n.compare(0, 3, "COM") == 0 || n.compare(0, 3, "LPT") == 0) &&
        n[3] >= '1' && n[3] <= '9') {
        return true;
    }
    return false;
}

std::string typeLetter(LocationType type) {
    switch (type) {
        case LocationType::Gate: return "G";
        case LocationType::Building: return "B";
        case LocationType::Facility: return "F";
    }
    return "F";
}

LocationType typeFromLetter(const std::string& letter) {
    if (letter == "G") return LocationType::Gate;
    if (letter == "B") return LocationType::Building;
    return LocationType::Facility;
}

// The type-specific text that CampusMap::addLocation() takes as `detail`.
std::string detailOf(const Location* loc) {
    switch (loc->kind()) {
        case LocationType::Building:
            return std::to_string(static_cast<const Building*>(loc)->floors());
        case LocationType::Facility:
            return static_cast<const Facility*>(loc)->service();
        case LocationType::Gate:
            return static_cast<const Gate*>(loc)->isOpen() ? "" : "closed";
    }
    return "";
}

}  // namespace

MapStore::MapStore(std::string directory) : dir_(std::move(directory)) {}

bool MapStore::validName(const std::string& name) {
    if (name.empty() || name.size() > 40) return false;
    for (char ch : name) {
        const unsigned char c = static_cast<unsigned char>(ch);
        if (!(std::isalnum(c) || ch == '_' || ch == '-')) return false;
    }
    return !isReservedDeviceName(name);
}

std::string MapStore::pathFor(const std::string& name) const {
    return (fs::path(dir_) / (name + ".json")).string();
}

bool MapStore::exists(const std::string& name) const {
    if (!validName(name)) return false;
    std::error_code ec;
    return fs::is_regular_file(pathFor(name), ec);
}

ds::DynamicArray<std::string> MapStore::list() const {
    ds::DynamicArray<std::string> names;
    std::error_code ec;
    fs::directory_iterator it(dir_, ec), end;
    if (ec) return names;  // no maps directory yet
    for (; it != end; it.increment(ec)) {
        if (ec) break;
        if (!it->is_regular_file(ec) || it->path().extension() != ".json") continue;
        const std::string stem = it->path().stem().string();
        if (validName(stem)) names.pushBack(stem);
    }
    // insertion sort (the list is tiny; avoids pulling in a std container)
    for (std::size_t i = 1; i < names.size(); ++i) {
        std::string key = names[i];
        std::size_t j = i;
        while (j > 0 && names[j - 1] > key) {
            names[j] = names[j - 1];
            --j;
        }
        names[j] = key;
    }
    return names;
}

bool MapStore::save(const std::string& name, const campus::CampusMap& map, std::string& error) const {
    if (!validName(name)) {
        error = "invalid map name (use letters, digits, '_' or '-', up to 40 characters)";
        return false;
    }

    Json root = Json::makeObject();
    root.members.put("version", Json::makeNumber(1));
    root.members.put("name", Json::makeString(name));
    root.members.put("width", Json::makeNumber(map.width()));
    root.members.put("height", Json::makeNumber(map.height()));

    Json locations = Json::makeArray();
    Json connections = Json::makeArray();
    for (const Location* loc : map.allLocations()) {
        Json obj = Json::makeObject();
        obj.members.put("id", Json::makeNumber(loc->id()));
        obj.members.put("name", Json::makeString(loc->name()));
        obj.members.put("type", Json::makeString(typeLetter(loc->kind())));
        obj.members.put("x", Json::makeNumber(loc->x()));
        obj.members.put("y", Json::makeNumber(loc->y()));
        obj.members.put("detail", Json::makeString(detailOf(loc)));
        locations.items.pushBack(obj);

        for (const Connection& conn : map.allConnectionsOf(loc->id())) {
            if (conn.toId <= loc->id()) continue;  // each undirected edge once
            Json edge = Json::makeObject();
            edge.members.put("from", Json::makeNumber(loc->id()));
            edge.members.put("to", Json::makeNumber(conn.toId));
            edge.members.put("d", Json::makeNumber(conn.distance));
            edge.members.put("label", Json::makeString(conn.label));
            connections.items.pushBack(edge);
        }
    }
    root.members.put("locations", locations);
    root.members.put("connections", connections);

    std::error_code ec;
    fs::create_directories(dir_, ec);
    if (ec) {
        error = "cannot create maps directory: " + ec.message();
        return false;
    }

    // Write to a temp file first so a crash mid-write cannot corrupt an existing save.
    const std::string target = pathFor(name);
    const std::string temp = target + ".tmp";
    {
        std::ofstream file(temp, std::ios::binary | std::ios::trunc);
        if (!file) {
            error = "cannot write " + temp;
            return false;
        }
        file << root.dump();
        file.flush();
        if (!file) {
            error = "write failed (disk full?)";
            return false;
        }
    }
    fs::remove(target, ec);  // rename() will not replace an existing file on every platform
    fs::rename(temp, target, ec);
    if (ec) {
        error = "cannot finalize save: " + ec.message();
        return false;
    }
    return true;
}

bool MapStore::load(const std::string& name, campus::CampusMap& out, std::string& error) const {
    if (!validName(name)) {
        error = "invalid map name";
        return false;
    }
    std::ifstream file(pathFor(name), std::ios::binary);
    if (!file) {
        error = "map '" + name + "' not found";
        return false;
    }
    std::ostringstream text;
    text << file.rdbuf();

    Json root;
    if (!Json::parse(text.str(), root) || root.type != Json::Type::Object) {
        error = "map file is not valid JSON";
        return false;
    }
    if (!out.reset(root.getInt("width", 0), root.getInt("height", 0))) {
        error = "map file has an invalid size";
        return false;
    }

    const Json* locations = root.get("locations");
    const Json* connections = root.get("connections");
    if (locations == nullptr || locations->type != Json::Type::Array || connections == nullptr ||
        connections->type != Json::Type::Array) {
        error = "map file is missing locations/connections";
        return false;
    }

    // Index the saved locations by id so they can be re-added in id order.
    int maxId = -1;
    for (const Json& item : locations->items) {
        const int id = item.getInt("id", -1);
        if (id < 0 || id > kMaxLocationId) {
            error = "map file has a location with a bad id";
            return false;
        }
        if (id > maxId) maxId = id;
    }
    ds::DynamicArray<const Json*> byId(static_cast<std::size_t>(maxId + 1), nullptr);
    for (const Json& item : locations->items) {
        const int id = item.getInt("id", -1);
        if (byId[static_cast<std::size_t>(id)] != nullptr) {
            error = "map file has two locations with the same id";
            return false;
        }
        byId[static_cast<std::size_t>(id)] = &item;
    }

    // CampusMap ids are slot indices, so a hole left by a deleted location is filled
    // with a throw-away placeholder and removed again once everything is in place.
    ds::DynamicArray<int> gaps;
    for (int id = 0; id <= maxId; ++id) {
        const Json* item = byId[static_cast<std::size_t>(id)];
        int added = -1;
        if (item != nullptr) {
            added = out.addLocation(item->getString("name"), typeFromLetter(item->getString("type")),
                                    item->getInt("x"), item->getInt("y"), item->getString("detail"));
        } else {
            added = out.addLocation("__gap_" + std::to_string(id), LocationType::Facility, 2, 1, "");
            gaps.pushBack(id);
        }
        if (added != id) {
            error = "map file has an empty or duplicate location name (id " + std::to_string(id) + ")";
            return false;
        }
    }
    for (const Json& edge : connections->items) {
        if (!out.connect(edge.getInt("from", -1), edge.getInt("to", -1), edge.getInt("d", 0),
                         edge.getString("label"))) {
            error = "map file has an invalid or duplicate connection";
            return false;
        }
    }
    for (int id : gaps) out.removeLocation(id);
    return true;
}

}  // namespace web
