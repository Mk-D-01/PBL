#include "Navigation.hpp"

#include "data_structures/HashMap.hpp"
#include "data_structures/MinHeap.hpp"
#include "data_structures/Queue.hpp"
#include "data_structures/Stack.hpp"

namespace campus {
namespace {

constexpr int kInvalid = -1;

// Rebuilds the id path from a predecessor map (Maps discovered node -> predecessor):
// walks back from the destination, then reverses.
ds::DynamicArray<int> buildPath(const ds::HashMap<int, int>& parent, int destination) {
    ds::DynamicArray<int> reversed;
    int walk = destination;
    while (walk != kInvalid) {
        reversed.pushBack(walk);
        const int* p = parent.find(walk);
        walk = (p == nullptr) ? kInvalid : *p;
    }
    ds::DynamicArray<int> path;
    for (std::size_t i = reversed.size(); i-- > 0;) path.pushBack(reversed[i]);
    return path;
}

}  // namespace

bool Navigation::valid(int id) const {
    return id >= 0 && id < map_.locationCount();
}

// ------------------------------------------------------------ BFS (fewest hops)

Route Navigation::shortestRouteByHops(int source, int destination) const {
    Route route;
    if (!valid(source) || !valid(destination)) return route;

    ds::Queue<int> frontier;
    ds::HashMap<int, int> parent;     // Maps discovered node -> predecessor
    ds::HashMap<int, bool> visited;

    parent.put(source, kInvalid);
    visited.put(source, true);
    frontier.enqueue(source);

    while (!frontier.empty()) {
        const int current = frontier.peek();
        frontier.dequeue();

        if (current == destination) {
            route.reachable = true;
            route.path = buildPath(parent, destination);
            route.hops = static_cast<int>(route.path.size()) - 1;
            for (std::size_t i = 1; i < route.path.size(); ++i) {
                route.totalDistance +=
                    map_.connectionDistance(route.path[i - 1], route.path[i]);
            }
            return route;
        }

        for (const Connection& c : map_.adjacency_[current]) {
            if (!visited.contains(c.toId)) {
                visited.put(c.toId, true);
                parent.put(c.toId, current);
                frontier.enqueue(c.toId);
            }
        }
    }
    return route;  // unreachable
}

// --------------------------------------------- Dijkstra (shortest by distance)

Route Navigation::shortestRouteByDistance(int source, int destination) const {
    Route route;
    if (!valid(source) || !valid(destination)) return route;

    ds::MinHeap<HeapEntry> open;      // priority queue (custom DS)
    ds::HashMap<int, int> dist;       // best known distance
    ds::HashMap<int, int> parent;     // Maps best node -> predecessor
    ds::HashMap<int, bool> done;      // finalized nodes

    dist.put(source, 0);
    open.push(HeapEntry(0, source));

    while (!open.empty()) {
        const HeapEntry top = open.extractMin();
        const int current = top.node;
        if (done.contains(current)) continue;  // stale queue entry
        done.put(current, true);

        if (current == destination) {
            route.reachable = true;
            route.totalDistance = top.dist;
            // Walk predecessors back from destination.
            ds::DynamicArray<int> reversed;
            int walk = destination;
            while (walk != kInvalid) {
                reversed.pushBack(walk);
                const int* p = parent.find(walk);
                walk = (p == nullptr) ? kInvalid : *p;
            }
            for (std::size_t i = reversed.size(); i-- > 0;) route.path.pushBack(reversed[i]);
            route.hops = static_cast<int>(route.path.size()) - 1;
            return route;
        }

        for (const Connection& c : map_.adjacency_[current]) {
            if (done.contains(c.toId)) continue;
            const long long candidate = static_cast<long long>(top.dist) + c.distance;
            const int* best = dist.find(c.toId);
            if (best == nullptr) {
                dist.put(c.toId, static_cast<int>(candidate));
                parent.put(c.toId, current);
                open.push(HeapEntry(static_cast<int>(candidate), c.toId));
            } else if (candidate < *best) {
                dist.put(c.toId, static_cast<int>(candidate));
                parent.put(c.toId, current);
                // Lower the key of the queue entry (falls back to a new entry
                // if the old one was already popped; stale entries are skipped
                // via `done` above).
                const HeapEntry updated(static_cast<int>(candidate), c.toId);
                if (!open.decreaseKey(HeapEntry(*best, c.toId), updated)) open.push(updated);
            }
        }
    }
    return route;  // unreachable
}

// ------------------------------------------------------------------- DFS path check

bool Navigation::pathExists(int source, int destination) const {
    if (!valid(source) || !valid(destination)) return false;
    if (source == destination) return true;

    ds::Stack<int> pending;
    ds::HashMap<int, bool> visited;
    pending.push(source);
    visited.put(source, true);

    while (!pending.empty()) {
        const int current = pending.top();
        pending.pop();
        if (current == destination) return true;

        for (const Connection& c : map_.adjacency_[current]) {
            if (!visited.contains(c.toId)) {
                visited.put(c.toId, true);
                pending.push(c.toId);
            }
        }
    }
    return false;
}

// ---------------------------------------------------------------- traversals

ds::DynamicArray<int> Navigation::traverseBFS(int source) const {
    ds::DynamicArray<int> order;
    if (!valid(source)) return order;

    ds::Queue<int> frontier;
    ds::HashMap<int, bool> visited;
    frontier.enqueue(source);
    visited.put(source, true);

    while (!frontier.empty()) {
        const int current = frontier.peek();
        frontier.dequeue();
        order.pushBack(current);
        for (const Connection& c : map_.adjacency_[current]) {
            if (!visited.contains(c.toId)) {
                visited.put(c.toId, true);
                frontier.enqueue(c.toId);
            }
        }
    }
    return order;
}

ds::DynamicArray<int> Navigation::traverseDFS(int source) const {
    ds::DynamicArray<int> order;
    if (!valid(source)) return order;

    ds::Stack<int> pending;
    ds::HashMap<int, bool> visited;
    pending.push(source);

    while (!pending.empty()) {
        const int current = pending.top();
        pending.pop();
        if (visited.contains(current)) continue;
        visited.put(current, true);
        order.pushBack(current);
        // Push neighbours in reverse so traversal visits them left-to-right.
        ds::DynamicArray<int> neighbours;
        for (const Connection& c : map_.adjacency_[current]) neighbours.pushBack(c.toId);
        for (std::size_t i = neighbours.size(); i-- > 0;) pending.push(neighbours[i]);
    }
    return order;
}

}  // namespace campus
