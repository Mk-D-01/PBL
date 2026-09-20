#include <cstring>
#include <iostream>
#include <string>

#include "Menu.hpp"

namespace {

// Runs one menu action by number; used by automated tests/demos so the
// whole UI layer can be exercised without a live console.
int runScripted(const std::string& action) {
    campus::Menu menu;
    if (action == "map") {
        menu.runAction(1);
        menu.runAction(2);
    } else if (action == "info") {
        menu.runAction(1);
        menu.runAction(9);
    } else if (action == "demo-route") {
        menu.runAction(1);
        menu.runAction(8);
    } else {
        std::cerr << "Unknown script action: " << action << "\n";
        return 2;
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    campus::Menu menu;
    if (argc >= 3 && std::strcmp(argv[1], "--script") == 0) return runScripted(argv[2]);
    menu.run();
    return 0;
}
