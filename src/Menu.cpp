#include "Menu.hpp"

#include <cctype>
#include <iostream>
#include <limits>
#include <sstream>

#if defined(_WIN32)
#include <io.h>
#define CAMPUS_ISATTY _isatty
#define CAMPUS_FILENO _fileno
#else
#include <unistd.h>
#define CAMPUS_ISATTY isatty
#define CAMPUS_FILENO fileno
#endif

namespace campus {
namespace {

bool inputIsInteractive() {
    return CAMPUS_ISATTY(CAMPUS_FILENO(stdin)) != 0;
}

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

int readIntLine(bool interactive) {
    std::string line;
    std::getline(std::cin, line);
    line = trim(line);
    try {
        return std::stoi(line);
    } catch (const std::exception&) {
        return -1;
    }
    (void)interactive;
}

// Reads an id or an exact location name and resolves it to an id (-1 on failure).
int resolveLocationRef(const std::string& token, CampusMap& map) {
    const std::string text = trim(token);
    if (text.empty()) return -1;
    try {
        std::size_t consumed = 0;
        const int id = std::stoi(text, &consumed);
        if (consumed == text.size() && map.findLocation(id) != nullptr) return id;
    } catch (const std::exception&) {
        // not a number -> fall through to name lookup
    }
    const Location* byName = map.searchByName(text);
    return byName == nullptr ? -1 : byName->id();
}

void printBanner() {
    std::cout << "\n================ MAPGEN ENGINE: CAMPUS LOCATOR ================\n";
    std::cout << " 1. Generate Campus Map\n";
    std::cout << " 2. Display Map\n";
    std::cout << " 3. Add Location\n";
    std::cout << " 4. Remove Location\n";
    std::cout << " 5. Search Location\n";
    std::cout << " 6. Update Location\n";
    std::cout << " 7. Add Connection\n";
    std::cout << " 8. Find Route\n";
    std::cout << " 9. Display Campus Information\n";
    std::cout << "10. Exit\n";
    std::cout << "===============================================================\n";
    std::cout << "Enter choice [1-10]: ";
}

}  // namespace

Menu::Menu() = default;

void Menu::printHeader() const {
    std::cout << "\n--- Campus Map (" << map_.locationCount() << " locations, "
              << map_.edgeCount() << " connections) ---\n";
}

// -------------------------------------------------------------------- 1

void Menu::generateCampusMap() {
    if (generated_) {
        std::cout << "Campus map already generated in this session.\n";
        return;
    }
    MapGenerator::generate(map_);
    generated_ = true;
    std::cout << "Campus map generated: " << map_.locationCount() << " locations, "
              << map_.edgeCount() << " connections.\n";
}

// -------------------------------------------------------------------- 2

void Menu::displayMap() {
    if (map_.locationCount() == 0) {
        std::cout << "Map is empty. Use option 1 first.\n";
        return;
    }
    std::cout << map_.renderMap();
    std::cout << "\nLocations:\n";
    for (const Location* loc : map_.allLocations()) {
        std::cout << "  [" << loc->id() << "] " << loc->describe() << "\n";
    }
}

// -------------------------------------------------------------------- 3

void Menu::addLocation() {
    std::cout << "Enter name: ";
    std::string name;
    std::getline(std::cin, name);
    name = trim(name);
    if (name.empty()) {
        std::cout << "Name cannot be empty.\n";
        return;
    }

    std::string typeToken, detail;
    promptLocationType(typeToken, detail);
    LocationType type;
    if (typeToken == "g") type = LocationType::Gate;
    else if (typeToken == "b") type = LocationType::Building;
    else if (typeToken == "f") type = LocationType::Facility;
    else {
        std::cout << "Invalid type.\n";
        return;
    }

    std::cout << "Enter x y (grid position, 0-based): ";
    const int x = readIntLine(inputIsInteractive());
    const int y = readIntLine(inputIsInteractive());

    const int id = map_.addLocation(name, type, x, y, detail);
    if (id < 0) {
        std::cout << "Could not add location (name already in use?).\n";
        return;
    }
    std::cout << "Added location '" << name << "' with id " << id << ".\n";
}

void Menu::promptLocationType(std::string& outType, std::string& outDetail) const {
    std::cout << "Enter type ((G)ate / (B)uilding / (F)acility): ";
    std::getline(std::cin, outType);
    outType = trim(outType);
    if (outType.size() == 1) outType = std::string(1, static_cast<char>(std::tolower(outType[0])));

    if (outType == "b") {
        std::cout << "Enter number of floors: ";
        std::string floors;
        std::getline(std::cin, floors);
        outDetail = trim(floors);
    } else if (outType == "f") {
        std::cout << "Enter service description: ";
        std::getline(std::cin, outDetail);
        outDetail = trim(outDetail);
    } else if (outType == "g") {
        std::cout << "Open gate? (yes/no, default yes): ";
        std::string open;
        std::getline(std::cin, open);
        outDetail = trim(open) == "no" ? "closed" : "open";
    }
}

// -------------------------------------------------------------------- 4

void Menu::removeLocation() {
    std::cout << "Enter location id or name: ";
    std::string token;
    std::getline(std::cin, token);
    int id = -1;
    try {
        id = resolveLocationRef(token, map_);
    } catch (const std::exception&) {
        id = -1;
    }
    if (id < 0) {
        std::cout << "Location not found.\n";
        return;
    }
    if (map_.removeLocation(id))
        std::cout << "Removed location (id " << id << ") and all its connections.\n";
    else
        std::cout << "Location not found.\n";
}

// -------------------------------------------------------------------- 5

void Menu::searchLocation() {
    std::cout << "Enter id/name for exact match, or keyword for partial search: ";
    std::string token;
    std::getline(std::cin, token);
    token = trim(token);

    if (const Location* exact = map_.searchByName(token)) {
        std::cout << "Found: [" << exact->id() << "] " << exact->describe() << "\n";
        return;
    }
    const ds::DynamicArray<Location*> hits = map_.searchByKeyword(token);
    if (hits.empty()) {
        std::cout << "No matching locations.\n";
        return;
    }
    std::cout << hits.size() << " match(es):\n";
    for (const Location* loc : hits) std::cout << "  [" << loc->id() << "] " << loc->describe() << "\n";
}

// -------------------------------------------------------------------- 6

void Menu::updateLocation() {
    std::cout << "Enter location id or name: ";
    std::string token;
    std::getline(std::cin, token);
    const int id = resolveLocationRef(token, map_);
    if (id < 0) {
        std::cout << "Location not found.\n";
        return;
    }

    std::cout << "Update (n)ame or (p)osition? ";
    std::string field;
    std::getline(std::cin, field);
    field = trim(field);

    if (field == "n" || field == "N") {
        std::cout << "Enter new name: ";
        std::string newName;
        std::getline(std::cin, newName);
        if (map_.updateLocation(id, trim(newName)))
            std::cout << "Name updated.\n";
        else
            std::cout << "Could not rename (empty or already in use).\n";
    } else if (field == "p" || field == "P") {
        std::cout << "Enter new x y: ";
        const int x = readIntLine(inputIsInteractive());
        const int y = readIntLine(inputIsInteractive());
        if (map_.updateLocation(id, x, y))
            std::cout << "Position updated.\n";
        else
            std::cout << "Location not found.\n";
    } else {
        std::cout << "Invalid choice.\n";
    }
}

// -------------------------------------------------------------------- 7

void Menu::addConnection() {
    if (map_.locationCount() < 2) {
        std::cout << "Need at least two locations first.\n";
        return;
    }
    std::cout << "Enter first location (id or name): ";
    std::string tokenA;
    std::getline(std::cin, tokenA);
    const int a = resolveLocationRef(tokenA, map_);

    std::cout << "Enter second location (id or name): ";
    std::string tokenB;
    std::getline(std::cin, tokenB);
    const int b = resolveLocationRef(tokenB, map_);

    if (a < 0 || b < 0) {
        std::cout << "Location(s) not found.\n";
        return;
    }

    std::cout << "Enter distance (metres): ";
    const int distance = readIntLine(inputIsInteractive());
    std::cout << "Enter label (walkway/road name, optional): ";
    std::string label;
    std::getline(std::cin, label);

    if (map_.connect(a, b, distance, trim(label)))
        std::cout << "Connected " << map_.findLocation(a)->name() << " <-> "
                  << map_.findLocation(b)->name() << " (" << distance << " m).\n";
    else
        std::cout << "Could not connect (same location, already connected, or bad distance).\n";
}

// -------------------------------------------------------------------- 8

void Menu::findRoute() {
    std::cout << "Enter source (id or name): ";
    std::string tokenA;
    std::getline(std::cin, tokenA);
    const int src = resolveLocationRef(tokenA, map_);

    std::cout << "Enter destination (id or name): ";
    std::string tokenB;
    std::getline(std::cin, tokenB);
    const int dst = resolveLocationRef(tokenB, map_);

    if (src < 0 || dst < 0) {
        std::cout << "Location(s) not found.\n";
        return;
    }

    std::cout << "Metric: (h)ops via BFS or (d)istance via Dijkstra? ";
    std::string metric;
    std::getline(std::cin, metric);
    metric = trim(metric);

    if (metric == "h" || metric == "H") {
        const Route route = Navigation(map_).shortestRouteByHops(src, dst);
        printRoute(route, "fewest stops (BFS)");
    } else if (metric == "d" || metric == "D") {
        const Route route = Navigation(map_).shortestRouteByDistance(src, dst);
        printRoute(route, "shortest distance (Dijkstra)");
    } else {
        std::cout << "Invalid metric.\n";
        return;
    }

    const bool connected = Navigation(map_).pathExists(src, dst);
    std::cout << "DFS connectivity check: " << (connected ? "path exists" : "no path") << ".\n";
}

void Menu::printRoute(const Route& route, const std::string& metric) const {
    if (!route.reachable) {
        std::cout << "No route available (" << metric << ").\n";
        return;
    }
    std::cout << "Route [" << metric << "]:\n";
    for (std::size_t i = 0; i < route.path.size(); ++i) {
        const Location* loc = map_.findLocation(route.path[i]);
        std::cout << "  " << (i + 1) << ". " << loc->name() << "\n";
    }
    std::cout << "  Stops: " << route.hops << ", Total distance: " << route.totalDistance
              << " m\n";
}

// -------------------------------------------------------------------- 9

void Menu::displayCampusInfo() {
    std::cout << map_.campusInfo();
    std::cout << "\nAdjacency list:\n";
    std::cout << map_.adjacencyReport();
}

// -------------------------------------------------------------------- loop

void Menu::pause() const {
    if (!inputIsInteractive()) return;
    std::cout << "\nPress ENTER to continue...";
    std::string dummy;
    std::getline(std::cin, dummy);
}

void Menu::runAction(int choice) {
    std::cout << '\n';
    switch (choice) {
        case 1: generateCampusMap(); break;
        case 2: displayMap(); break;
        case 3: addLocation(); break;
        case 4: removeLocation(); break;
        case 5: searchLocation(); break;
        case 6: updateLocation(); break;
        case 7: addConnection(); break;
        case 8: findRoute(); break;
        case 9: displayCampusInfo(); break;
        case 10: break;
        default: std::cout << "Invalid choice. Enter a number from 1 to 10.\n"; break;
    }
}

void Menu::run() {
    bool running = true;
    while (running) {
        printBanner();
        const int choice = readIntLine(inputIsInteractive());
        if (choice == 10) {
            running = false;
            break;
        }
        runAction(choice);
        if (running) pause();
    }
    std::cout << "Goodbye!\n";
}

}  // namespace campus
