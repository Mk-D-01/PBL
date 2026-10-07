#pragma once

#include <string>

#include "CampusMap.hpp"
#include "data_structures/DynamicArray.hpp"

namespace web {

// MapStore: saves/loads whole maps as one JSON file each (<directory>/<name>.json).
//
// File format (version 1):
//   {"version":1,"name":"...","width":62,"height":20,
//    "locations":[{"id":0,"name":"MainGate","type":"G","x":3,"y":18,"detail":""},...],
//    "connections":[{"from":0,"to":2,"d":90,"label":"Main Walkway"},...]}
// Location ids are stored as-is (gaps from deleted locations included), so ids
// stay stable across save/load - routes and connections keep their meaning.
class MapStore {
public:
    explicit MapStore(std::string directory = "maps");

    // Map names become file names, so they are restricted to [A-Za-z0-9_-], 1..40
    // chars, and must not be a Windows reserved device name (CON, NUL, COM1, ...).
    static bool validName(const std::string& name);

    bool exists(const std::string& name) const;
    // Names of all saved maps, sorted alphabetically.
    ds::DynamicArray<std::string> list() const;

    // Both return false and fill `error` on failure. load() expects a fresh map and
    // leaves it half-built on failure, so the caller must discard it in that case.
    bool save(const std::string& name, const campus::CampusMap& map, std::string& error) const;
    bool load(const std::string& name, campus::CampusMap& out, std::string& error) const;

private:
    std::string pathFor(const std::string& name) const;

    std::string dir_;
};

}  // namespace web
