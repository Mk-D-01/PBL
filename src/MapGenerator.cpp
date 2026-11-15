#include "MapGenerator.hpp"

#include <stdexcept>

namespace campus {
namespace {

// Adds a location and aborts (assert-style) if the seed data is inconsistent.
int addOrDie(CampusMap& map, const std::string& name, LocationType type, int x, int y,
             const std::string& detail) {
    const int id = map.addLocation(name, type, x, y, detail);
    if (id < 0) throw std::runtime_error("MapGenerator: duplicate seed name '" + name + "'");
    return id;
}

}  // namespace

// Coordinates were read off the sketches in frontWiew/ at 20 px per grid unit
// (Block B is shifted down below Block A). Distances are in metres, taking
// 10 px of sketch = 1 m and measuring centre to centre along the corridor.
void MapGenerator::generate(CampusMap& map) {
    map.reset(SEED_WIDTH, SEED_HEIGHT);  // blank map at the seed layout's size

    const LocationType F = LocationType::Facility;

    // ------------------------------------------------- Block A (er.png)
    // ids are assigned in creation order: A-Entrance = 0 (MAIN_GATE_ID)
    const int aEntrance = addOrDie(map, "A-Entrance",  LocationType::Gate, 31, 39, "");
    const int aStairsNW = addOrDie(map, "A-Stairs-NW", F, 10,  5, "Staircase");
    const int aCr1      = addOrDie(map, "A-Cr1",       F, 16,  7, "Classroom");
    const int aCr2      = addOrDie(map, "A-Cr2",       F, 23,  7, "Classroom");
    const int aCr3      = addOrDie(map, "A-Cr3",       F, 30,  7, "Classroom");
    const int aCr4      = addOrDie(map, "A-Cr4",       F, 39,  7, "Classroom");
    const int aCr5      = addOrDie(map, "A-Cr5",       F, 46,  7, "Classroom");
    const int aStairsNE = addOrDie(map, "A-Stairs-NE", F, 52,  4, "Staircase");
    const int aLabW     = addOrDie(map, "A-Lab-W",     F, 17, 22, "Computer Lab");
    const int aLabE     = addOrDie(map, "A-Lab-E",     F, 45, 22, "Computer Lab");
    const int wifiZone  = addOrDie(map, "WiFi Zone",   F, 31, 22, "Open seating, Wi-Fi");
    const int aStairsS1 = addOrDie(map, "A-Stairs-S1", F, 25, 33, "Staircase");
    const int aLift     = addOrDie(map, "A-Lift",      F, 31, 33, "Lift");
    const int aStairsS2 = addOrDie(map, "A-Stairs-S2", F, 37, 33, "Staircase");
    const int aCornerSW = addOrDie(map, "A-Corner-SW", F, 11, 39, "Corridor");
    const int aCornerSE = addOrDie(map, "A-Corner-SE", F, 51, 39, "Corridor");

    // ------------------------------------------------- Block B (screenshot)
    const int bEntrance = addOrDie(map, "B-Entrance",      LocationType::Gate, 31, 45, "");
    const int bLtNW     = addOrDie(map, "B-LT-NW",         F, 11, 47, "Lecture Theatre");
    const int bCr1      = addOrDie(map, "B-Cr1",           F, 22, 47, "Classroom");
    const int bCr2      = addOrDie(map, "B-Cr2",           F, 42, 47, "Classroom");
    const int bLtNE     = addOrDie(map, "B-LT-NE",         F, 51, 47, "Lecture Theatre");
    const int bStairsW  = addOrDie(map, "B-Stairs-W",      F, 18, 52, "Staircase");
    const int bStairsE  = addOrDie(map, "B-Stairs-E",      F, 43, 53, "Staircase");
    const int openAudi  = addOrDie(map, "Open Audi",       F, 31, 64, "Open auditorium");
    const int maleWash  = addOrDie(map, "Male WashRoom",   F, 10, 61, "Washroom (male)");
    const int bCr3      = addOrDie(map, "B-Cr3",           F, 51, 56, "Classroom");
    const int bCr4      = addOrDie(map, "B-Cr4",           F, 51, 67, "Classroom");
    const int bLtSW     = addOrDie(map, "B-LT-SW",         F, 11, 79, "Lecture Theatre");
    const int femaleWash= addOrDie(map, "Female WashRoom", F, 31, 80, "Washroom (female)");
    const int bLtSE     = addOrDie(map, "B-LT-SE",         F, 51, 79, "Lecture Theatre");

    // ------------------------------------------------- corridors
    struct Edge { int a, b, dist; const char* label; };
    const Edge edges[] = {
        // Block A: outer corridor loop (rooms sit on it, so it is a chain of stops)
        {aStairsNW, aCr1,      12, "A Top Corridor"},
        {aCr1,      aCr2,      14, "A Top Corridor"},
        {aCr2,      aCr3,      15, "A Top Corridor"},
        {aCr3,      aCr4,      17, "A Top Corridor"},
        {aCr4,      aCr5,      16, "A Top Corridor"},
        {aCr5,      aStairsNE, 12, "A Top Corridor"},
        {aStairsNW, aLabW,     37, "A West Corridor"},
        {aLabW,     aCornerSW, 36, "A West Corridor"},
        {aStairsNE, aLabE,     37, "A East Corridor"},
        {aLabE,     aCornerSE, 36, "A East Corridor"},
        {aCornerSW, aEntrance, 40, "A South Corridor"},
        {aEntrance, aCornerSE, 40, "A South Corridor"},
        // Block A: central Wi-Fi zone touches both labs and the stairs/lift lobby
        {wifiZone,  aLabW,     28, "WiFi Zone"},
        {wifiZone,  aLabE,     29, "WiFi Zone"},
        {wifiZone,  aStairsS1, 25, "WiFi Zone"},
        {wifiZone,  aLift,     22, "WiFi Zone"},
        {wifiZone,  aStairsS2, 25, "WiFi Zone"},
        {aStairsS1, aEntrance, 17, "Lobby"},
        {aLift,     aEntrance, 12, "Lobby"},
        {aStairsS2, aEntrance, 17, "Lobby"},
        // the passage joining the two blocks
        {aEntrance, bEntrance, 13, "Link Passage"},
        // Block B: outer corridor loop
        {bLtNW,     bCr1,      21, "B North Corridor"},
        {bCr1,      bEntrance, 18, "B North Corridor"},
        {bEntrance, bCr2,      23, "B North Corridor"},
        {bCr2,      bLtNE,     18, "B North Corridor"},
        {bLtNE,     bCr3,      18, "B East Corridor"},
        {bCr3,      bCr4,      22, "B East Corridor"},
        {bCr4,      bLtSE,     23, "B East Corridor"},
        {bLtSE,     femaleWash,40, "B South Corridor"},
        {femaleWash,bLtSW,     40, "B South Corridor"},
        {bLtSW,     maleWash,  35, "B West Corridor"},
        {maleWash,  bLtNW,     28, "B West Corridor"},
        // Block B: stairs at the Open Audi's top corners lead in from the corridor
        {bStairsW,  bLtNW,     16, "Stair Landing"},
        {bStairsW,  bCr1,      13, "Stair Landing"},
        {bStairsE,  bCr2,      11, "Stair Landing"},
        {bStairsE,  bLtNE,     19, "Stair Landing"},
        {bStairsW,  openAudi,  35, "Audi Aisle"},
        {bStairsE,  openAudi,  34, "Audi Aisle"},
    };
    for (const Edge& e : edges) map.connect(e.a, e.b, e.dist, e.label);
}

}  // namespace campus
