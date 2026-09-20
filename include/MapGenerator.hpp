#pragma once

#include "CampusMap.hpp"

namespace campus {

// MapGenerator: builds the predefined demo campus (milestone 1 & 4).
// Nodes = gates / buildings / facilities; edges = roads & walkways.
class MapGenerator {
public:
    // Populates `map` with the default campus layout.
    // Returns nothing; ids of interest are exposed as static constants.
    static void generate(CampusMap& map);

    static constexpr int MAIN_GATE_ID = 0;
};

}  // namespace campus
