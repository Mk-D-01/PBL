#pragma once

#include <string>

namespace campus {

// Connection: an undirected edge between two locations.
// distance is the edge weight (metres); label is the walkway/road name.
struct Connection {
    int toId = -1;
    int distance = 1;
    std::string label;

    Connection() = default;
    Connection(int to, int distance, const std::string& label)
        : toId(to), distance(distance), label(label) {}
};

}  // namespace campus
