#include "MapGenerator.hpp"

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

void MapGenerator::generate(CampusMap& map) {
    map.clear();  // re-seeding always starts from a blank map

    // ------------------------------------------------- location placement
    // ids are assigned in creation order: 0..N-1
    const int mainGate   = addOrDie(map, "MainGate",   LocationType::Gate,     3,  18, "");
    const int eastGate   = addOrDie(map, "EastGate",   LocationType::Gate,    55,   3, "");
    const int cse        = addOrDie(map, "CSE-Block",  LocationType::Building, 6,  10, "5");
    const int ece        = addOrDie(map, "ECE-Block",  LocationType::Building, 18,  6, "4");
    const int mech       = addOrDie(map, "ME-Block",   LocationType::Building, 30,  5, "3");
    const int admin      = addOrDie(map, "Admin-Block",LocationType::Building, 14, 14, "2");
    const int library    = addOrDie(map, "Library",    LocationType::Facility, 26, 11, "Books & Reading Hall");
    const int cafeteria  = addOrDie(map, "Cafeteria",  LocationType::Facility, 38, 13, "Food Court");
    const int hostel     = addOrDie(map, "Hostel-A",   LocationType::Building, 46, 17, "6");
    const int medical    = addOrDie(map, "MedicalCtr", LocationType::Facility, 50,  9, "First Aid & Pharmacy");
    const int sports     = addOrDie(map, "SportsField",LocationType::Facility, 22,  3, "Ground & Track");
    const int parking    = addOrDie(map, "Parking",    LocationType::Facility,  8,  4, "Two-Wheeler & Car");

    // ------------------------------------------------- roads and walkways
    struct Edge { int a, b, dist; const char* label; };
    const Edge edges[] = {
        {mainGate,  parking,   60, "Entry Road"},
        {mainGate,  cse,       90, "Main Walkway"},
        {mainGate,  admin,     80, "Admin Road"},
        {parking,   cse,       70, "Side Path"},
        {parking,   sports,   110, "North Path"},
        {sports,    ece,       90, "Field Path"},
        {cse,       ece,       80, "Academic Walk"},
        {cse,       library,  100, "Library Lane"},
        {cse,       admin,     60, "Office Path"},
        {ece,       mech,      70, "Academic Walk"},
        {ece,       library,   90, "Reading Path"},
        {mech,      eastGate, 140, "East Road"},
        {mech,      library,   80, "Central Path"},
        {library,   cafeteria, 70, "Cafeteria Lane"},
        {admin,     cafeteria, 120, "Service Road"},
        {cafeteria, hostel,    90, "Hostel Road"},
        {cafeteria, medical,   70, "Clinic Path"},
        {hostel,    medical,   90, "Hostel Path"},
        {hostel,    eastGate, 120, "East Road"},
        {sports,    mech,     100, "North-South Path"},
    };
    for (const Edge& e : edges) map.connect(e.a, e.b, e.dist, e.label);
}

}  // namespace campus
