#include "CampusMap.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace campus {
namespace {

std::string toLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return text;
}

bool containsIgnoreCase(const std::string& haystack, const std::string& needle) {
    return toLower(haystack).find(toLower(needle)) != std::string::npos;
}

// Clamps coordinates so names and markers stay inside the grid.
void clampCoords(int& x, int& y) {
    if (x < 2) x = 2;
    if (x > CampusMap::GRID_WIDTH - 3) x = CampusMap::GRID_WIDTH - 3;
    if (y < 1) y = 1;
    if (y > CampusMap::GRID_HEIGHT - 2) y = CampusMap::GRID_HEIGHT - 2;
}

// Removes the edge towards `toId` from one adjacency bucket (at most one
// exists, since duplicate connections are rejected in connect()).
bool removeEdgeFrom(ds::LinkedList<Connection>& bucket, int toId) {
    return bucket.removeFirstIf([toId](const Connection& c) { return c.toId == toId; });
}

}  // namespace

CampusMap::CampusMap() : grid_(GRID_HEIGHT, ds::DynamicArray<char>(GRID_WIDTH, ' ')) {}

CampusMap::~CampusMap() {
    for (Location* loc : locations_) delete loc;
}

void CampusMap::clear() {
    for (Location* loc : locations_) delete loc;
    locations_.clear();
    adjacency_.clear();
    nameToId_.clear();
    edgeCount_ = 0;
    liveCount_ = 0;
    gridDirty_ = true;
}

// ---------------------------------------------------------------- locations

int CampusMap::addLocation(const std::string& name, LocationType type, int x, int y,
                           const std::string& detail) {
    if (name.empty() || searchByName(name) != nullptr) return -1;
    clampCoords(x, y);

    const int id = static_cast<int>(locations_.size());
    Location* loc = nullptr;
    switch (type) {
        case LocationType::Building: {
            int floors = 1;
            try {
                floors = std::stoi(detail);
            } catch (const std::exception&) {
                floors = 1;
            }
            loc = new Building(id, name, x, y, floors);
            break;
        }
        case LocationType::Facility:
            loc = new Facility(id, name, x, y, detail.empty() ? "general" : detail);
            break;
        case LocationType::Gate:
            loc = new Gate(id, name, x, y, detail != "closed");
            break;
    }

    locations_.pushBack(loc);
    adjacency_.pushBack(ds::LinkedList<Connection>());
    nameToId_.put(name, id);
    ++liveCount_;
    gridDirty_ = true;
    return id;
}

bool CampusMap::removeLocation(int id) {
    Location* loc = findLocation(id);
    if (loc == nullptr) return false;

    adjacency_[id].clear();  // drop this node's outgoing half-edges
    for (ds::LinkedList<Connection>& bucket : adjacency_) {
        while (removeEdgeFrom(bucket, id)) --edgeCount_;
    }

    nameToId_.remove(loc->name());
    delete loc;
    locations_[id] = nullptr;  // slot stays so remaining ids remain stable
    --liveCount_;
    gridDirty_ = true;
    return true;
}

bool CampusMap::updateLocation(int id, const std::string& newName) {
    Location* loc = findLocation(id);
    if (loc == nullptr || newName.empty()) return false;

    const int* clash = nameToId_.find(newName);
    if (clash != nullptr && *clash != id) return false;  // name must stay unique

    nameToId_.remove(loc->name());
    loc->rename(newName);
    nameToId_.put(newName, id);
    gridDirty_ = true;
    return true;
}

bool CampusMap::updateLocation(int id, int newX, int newY) {
    Location* loc = findLocation(id);
    if (loc == nullptr) return false;
    clampCoords(newX, newY);
    loc->moveTo(newX, newY);
    gridDirty_ = true;
    return true;
}

Location* CampusMap::findLocation(int id) const {
    if (id < 0 || id >= static_cast<int>(locations_.size())) return nullptr;
    return locations_[id];
}

Location* CampusMap::searchByName(const std::string& name) const {
    const int* id = nameToId_.find(name);
    return id == nullptr ? nullptr : findLocation(*id);
}

ds::DynamicArray<Location*> CampusMap::searchByKeyword(const std::string& keyword) const {
    ds::DynamicArray<Location*> hits;
    if (keyword.empty()) return hits;
    for (Location* loc : locations_) {
        if (loc != nullptr && containsIgnoreCase(loc->describe(), keyword)) hits.pushBack(loc);
    }
    return hits;
}

ds::DynamicArray<Location*> CampusMap::allLocations() const {
    ds::DynamicArray<Location*> result;
    for (Location* loc : locations_)
        if (loc != nullptr) result.pushBack(loc);
    return result;
}

// --------------------------------------------------------------- connections

bool CampusMap::connect(int aId, int bId, int distance, const std::string& label) {
    if (findLocation(aId) == nullptr || findLocation(bId) == nullptr) return false;
    if (aId == bId || distance <= 0) return false;
    if (areConnected(aId, bId)) return false;  // simple graph: no parallel edges

    adjacency_[aId].pushBack(Connection(bId, distance, label));
    adjacency_[bId].pushBack(Connection(aId, distance, label));
    ++edgeCount_;
    gridDirty_ = true;
    return true;
}

bool CampusMap::disconnect(int aId, int bId) {
    if (findLocation(aId) == nullptr || findLocation(bId) == nullptr) return false;
    bool removed = removeEdgeFrom(adjacency_[aId], bId);
    if (removed) removeEdgeFrom(adjacency_[bId], aId);
    if (removed) {
        --edgeCount_;
        gridDirty_ = true;
    }
    return removed;
}

bool CampusMap::areConnected(int aId, int bId) const {
    if (aId < 0 || aId >= static_cast<int>(adjacency_.size())) return false;
    return adjacency_[aId].containsIf(
        [bId](const Connection& c) { return c.toId == bId; });
}

int CampusMap::connectionDistance(int aId, int bId) const {
    if (aId < 0 || aId >= static_cast<int>(adjacency_.size())) return -1;
    for (const Connection& c : adjacency_[aId])
        if (c.toId == bId) return c.distance;
    return -1;
}

const ds::LinkedList<Connection>& CampusMap::allConnectionsOf(int id) const {
    static const ds::LinkedList<Connection> empty;
    if (id < 0 || id >= static_cast<int>(adjacency_.size())) return empty;
    return adjacency_[id];
}

// ---------------------------------------------------------------- rendering

void CampusMap::putGlyph(ds::DynamicArray<ds::DynamicArray<char>>& grid, int x, int y, char ch) {
    if (y < 0 || y >= static_cast<int>(grid.size())) return;
    if (x < 0 || x >= static_cast<int>(grid[y].size())) return;
    grid[y][x] = ch;
}

void CampusMap::plotLine(int x0, int y0, int x1, int y1, char brush) {
    // Bresenham's line algorithm (integer-only, hand-written).
    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        putGlyph(grid_, x0, y0, brush);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void CampusMap::drawEdges() {
    for (int id = 0; id < static_cast<int>(locations_.size()); ++id) {
        const Location* loc = locations_[id];
        if (loc == nullptr) continue;
        for (const Connection& c : adjacency_[id]) {
            if (c.toId <= id) continue;  // draw each undirected edge once
            const Location* other = locations_[c.toId];
            if (other == nullptr) continue;
            plotLine(loc->x(), loc->y(), other->x(), other->y(), '.');
        }
    }
}

void CampusMap::drawNode(const Location* loc) {
    const char marker = loc->kind() == LocationType::Gate    ? '*'
                        : loc->kind() == LocationType::Building ? 'B'
                                                               : 'F';
    putGlyph(grid_, loc->x() - 1, loc->y(), marker);
    const std::string& name = loc->name();
    for (std::size_t i = 0; i < name.size(); ++i)
        putGlyph(grid_, loc->x() + static_cast<int>(i), loc->y(), name[i]);
}

void CampusMap::rebuildGrid() {
    grid_ = ds::DynamicArray<ds::DynamicArray<char>>(GRID_HEIGHT,
                                                     ds::DynamicArray<char>(GRID_WIDTH, ' '));
    drawEdges();
    for (const Location* loc : locations_)
        if (loc != nullptr) drawNode(loc);
    gridDirty_ = false;
}

std::string CampusMap::renderMap() const {
    if (gridDirty_) const_cast<CampusMap*>(this)->rebuildGrid();
    std::ostringstream out;
    const std::string border(GRID_WIDTH, '-');
    out << '+' << border << "+\n";
    for (int row = 0; row < GRID_HEIGHT; ++row) {
        out << '|';
        for (int col = 0; col < GRID_WIDTH; ++col) out << grid_[row][col];
        out << "|\n";
    }
    out << '+' << border << "+\n";
    out << "Legend: * = Gate   B = Building   F = Facility   . = walkway\n";
    return out.str();
}

// ------------------------------------------------------------------ reports

std::string CampusMap::adjacencyReport() const {
    std::ostringstream out;
    for (const Location* loc : locations_) {
        if (loc == nullptr) continue;
        out << loc->describe() << "\n";
        bool any = false;
        for (const Connection& c : adjacency_[loc->id()]) {
            const Location* neighbour = findLocation(c.toId);
            if (neighbour == nullptr) continue;
            out << "    -> " << neighbour->name() << "  (" << c.distance << " m";
            if (!c.label.empty()) out << " via " << c.label;
            out << ")\n";
            any = true;
        }
        if (!any) out << "    (no connections)\n";
    }
    return out.str();
}

std::string CampusMap::campusInfo() const {
    int buildings = 0, facilities = 0, gates = 0, openGates = 0;
    std::string gateList;
    for (const Location* loc : locations_) {
        if (loc == nullptr) continue;
        switch (loc->kind()) {
            case LocationType::Building: ++buildings; break;
            case LocationType::Facility: ++facilities; break;
            case LocationType::Gate: {
                ++gates;
                const Gate* gate = static_cast<const Gate*>(loc);
                if (gate->isOpen()) {
                    ++openGates;
                    if (!gateList.empty()) gateList += ", ";
                    gateList += gate->name();
                }
                break;
            }
        }
    }
    std::ostringstream out;
    out << "Campus Information\n";
    out << "------------------\n";
    out << "Locations   : " << liveCount_ << " (Buildings: " << buildings
        << ", Facilities: " << facilities << ", Gates: " << gates << ")\n";
    out << "Connections : " << edgeCount_ << "\n";
    out << "Open gates  : " << openGates << (openGates > 0 ? " (" + gateList + ")" : "")
        << "\n";
    return out.str();
}

}  // namespace campus
