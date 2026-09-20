#pragma once

#include <string>

#include "CampusMap.hpp"
#include "MapGenerator.hpp"
#include "Navigation.hpp"

namespace campus {

// Menu: the user interface module (milestone 6).
// Menu-driven console interface with the 10 options from the proposal.
class Menu {
public:
    Menu();

    // Runs the interactive loop until the user chooses Exit.
    void run();

    // Executes one menu action by number (1-10); used by scripts/tests.
    void runAction(int choice);

private:
    // ---- menu actions (option numbers match the proposal) ----
    void generateCampusMap();
    void displayMap();
    void addLocation();
    void removeLocation();
    void searchLocation();
    void updateLocation();
    void addConnection();
    void findRoute();
    void displayCampusInfo();

    // ---- helpers ----
    void printHeader() const;
    void promptLocationType(std::string& outType, std::string& outDetail) const;
    bool readLocationRef(int& outId);          // id or exact name -> id
    void printRoute(const Route& route, const std::string& metric) const;
    void pause() const;

    CampusMap map_;
    bool generated_ = false;
};

}  // namespace campus
