#pragma once

#include "CampusMap.hpp"

namespace campus {

// MapGenerator: builds the predefined default map (milestone 1 & 4).
// The layout follows the floor-plan sketches in frontWiew/:
//   Block A (er.png)         - 5 classrooms, 2 labs, Wi-Fi zone, lift, 4 staircases
//   Block B (Screenshot ...) - 4 lecture theatres, 4 classrooms, Open Audi, washrooms
// joined by a short passage between A's bottom entrance and B's top entrance.
// Nodes = rooms / stairs / corridor corners / entrances; edges = corridor segments.
class MapGenerator {
public:
    // Clears `map`, resizes it to SEED_WIDTH x SEED_HEIGHT and seeds the layout.
    static void generate(CampusMap& map);

    // The two blocks are stacked vertically, so the seed map is tall and narrow.
    static constexpr int SEED_WIDTH = 58;
    static constexpr int SEED_HEIGHT = 84;

    static constexpr int MAIN_GATE_ID = 0;  // "A-Entrance"
};

}  // namespace campus
