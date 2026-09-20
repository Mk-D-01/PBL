#pragma once

#include <string>

#include "Connection.hpp"
#include "data_structures/DynamicArray.hpp"
#include "data_structures/Hash.hpp"

namespace campus {

// HeapEntry: value stored in ds::MinHeap during Dijkstra.
// Ordered by distance (the "key"); ties broken by node id for determinism.
struct HeapEntry {
    int dist;
    int node;

    HeapEntry() : dist(0), node(-1) {}
    HeapEntry(int d, int n) : dist(d), node(n) {}

    bool operator<(const HeapEntry& other) const {
        if (dist != other.dist) return dist < other.dist;
        return node < other.node;
    }

    bool operator==(const HeapEntry& other) const {
        return dist == other.dist && node == other.node;
    }
};

// Route: result of a navigation query.
struct Route {
    bool reachable = false;
    int totalDistance = 0;       // sum of edge weights along the path
    int hops = 0;                // number of edges (fewest-stops metric)
    ds::DynamicArray<int> path;  // location ids from source to destination

    Route() = default;
};

}  // namespace campus
