#pragma once

#include <string>

#include "Connection.hpp"
#include "Location.hpp"
#include "Route.hpp"
#include "data_structures/DynamicArray.hpp"
#include "data_structures/HashMap.hpp"

namespace campus {

// CampusMap: the graph engine.
//
// Data structures (all hand-implemented in ds::):
//   - locations_   : DynamicArray<Location*>  (id-indexed node table)
//   - nameToId_    : HashMap<std::string,int> (name -> id lookup, O(1) average)
//   - adjacency_   : DynamicArray<LinkedList<Connection>> (one bucket per id)
//   - grid_        : 2-D char grid for the simplified ASCII campus map
//
// Responsibilities: add / remove / update / search locations, connect them,
// display information, and render the map.
class CampusMap {
public:
    // Default map size (used by the console app and the generated campus).
    static constexpr int GRID_WIDTH = 62;
    static constexpr int GRID_HEIGHT = 20;
    // Allowed size range for maps created at runtime.
    static constexpr int MIN_WIDTH = 20, MAX_WIDTH = 200;
    static constexpr int MIN_HEIGHT = 10, MAX_HEIGHT = 100;

    explicit CampusMap(int width = GRID_WIDTH, int height = GRID_HEIGHT);
    ~CampusMap();

    CampusMap(const CampusMap&) = delete;
    CampusMap& operator=(const CampusMap&) = delete;

    // Removes every location and connection; ids restart from 0. Keeps the size.
    void clear();
    // clear() plus a new map size. Returns false (and changes nothing) when the
    // size is outside MIN_/MAX_WIDTH/HEIGHT.
    bool reset(int width, int height);

    int width() const { return width_; }
    int height() const { return height_; }
    // Inclusive coordinate range a location may occupy (names/markers stay inside the grid).
    int minX() const { return 2; }
    int maxX() const { return width_ - 3; }
    int minY() const { return 1; }
    int maxY() const { return height_ - 2; }

    // ---- location management ----
    // Creates the location object of the subtype implied by `type` and adds it
    // to the graph. Returns the new id, or -1 when the name is taken/invalid.
    int addLocation(const std::string& name, LocationType type, int x, int y,
                    const std::string& detail);
    bool removeLocation(int id);
    bool updateLocation(int id, const std::string& newName);
    bool updateLocation(int id, int newX, int newY);
    Location* findLocation(int id) const;              // by id
    Location* searchByName(const std::string& name) const;  // by exact name
    ds::DynamicArray<Location*> searchByKeyword(const std::string& keyword) const;
    ds::DynamicArray<Location*> allLocations() const;
    int locationCount() const { return liveCount_; }

    // ---- connections (edges) ----
    bool connect(int aId, int bId, int distance, const std::string& label);
    bool disconnect(int aId, int bId);
    bool areConnected(int aId, int bId) const;
    int connectionDistance(int aId, int bId) const;  // -1 when not adjacent
    int edgeCount() const { return edgeCount_; }
    // Read-only view of a location's adjacency bucket (empty for invalid ids).
    const ds::LinkedList<Connection>& allConnectionsOf(int id) const;

    // ---- graph & map output ----
    // Renders the simplified ASCII campus map into a single string.
    std::string renderMap() const;
    // Multi-line listing of every location with its adjacency list.
    std::string adjacencyReport() const;
    // Summary statistics of the whole campus.
    std::string campusInfo() const;

private:
    friend class Navigation;

    void clampCoords(int& x, int& y) const;
    void rebuildGrid();
    void drawNode(const Location* loc);
    void drawEdges();
    void plotLine(int x0, int y0, int x1, int y1, char brush);
    static void putGlyph(ds::DynamicArray<ds::DynamicArray<char>>& grid, int x, int y, char ch);

    // Locations by id; entries stay (as nullptr) after removal to keep ids stable.
    ds::DynamicArray<Location*> locations_;
    ds::HashMap<std::string, int> nameToId_;
    ds::DynamicArray<ds::LinkedList<Connection>> adjacency_;
    int edgeCount_ = 0;
    int liveCount_ = 0;  // locations actually present (slots may be freed)

    int width_;
    int height_;
    ds::DynamicArray<ds::DynamicArray<char>> grid_;
    bool gridDirty_ = true;
};

}  // namespace campus
