#pragma once

#include "CampusMap.hpp"
#include "Route.hpp"
#include "data_structures/DynamicArray.hpp"

namespace campus {

// Navigation: graph traversal and route finding over a CampusMap (milestone 5).
class Navigation {
public:
    explicit Navigation(const CampusMap& map) : map_(map) {}

    // BFS: shortest route by number of edges ("fewest stops").
    Route shortestRouteByHops(int source, int destination) const;

    // Dijkstra: shortest route by total distance (edge weights), using ds::MinHeap.
    Route shortestRouteByDistance(int source, int destination) const;

    // DFS: returns true when a path exists (connectivity check).
    bool pathExists(int source, int destination) const;

    // BFS traversal order from `source` (empty if source invalid).
    ds::DynamicArray<int> traverseBFS(int source) const;

    // DFS (iterative, using ds::Stack) traversal order from `source`.
    ds::DynamicArray<int> traverseDFS(int source) const;

private:
    bool valid(int id) const;

    const CampusMap& map_;
};

}  // namespace campus
