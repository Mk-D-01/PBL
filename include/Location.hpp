#pragma once

#include <string>

#include "data_structures/DynamicArray.hpp"

namespace campus {

// Kind of campus location; drives the polymorphic subtype used by CampusMap.
enum class LocationType { Gate, Building, Facility };

inline const char* toString(LocationType type) {
    switch (type) {
        case LocationType::Gate: return "Gate";
        case LocationType::Building: return "Building";
        case LocationType::Facility: return "Facility";
    }
    return "Unknown";
}

// Location: abstract base class of the OOP hierarchy (encapsulation + polymorphism).
// Derived classes (Building / Facility / Gate) override describe() and kind().
class Location {
public:
    Location(int id, std::string name, int x, int y)
        : id_(id), name_(std::move(name)), x_(x), y_(y) {}
    virtual ~Location() = default;

    // Polymorphic interface.
    virtual LocationType kind() const = 0;
    virtual std::string describe() const = 0;

    // Encapsulated accessors / mutators (no public data members).
    int id() const { return id_; }
    const std::string& name() const { return name_; }
    int x() const { return x_; }
    int y() const { return y_; }

    void rename(const std::string& name) { name_ = name; }
    void moveTo(int x, int y) {
        x_ = x;
        y_ = y;
    }

private:
    int id_;
    std::string name_;
    int x_;  // grid column used by the ASCII map
    int y_;  // grid row used by the ASCII map
};

// Building: academic/administrative block, hostel, etc.
class Building : public Location {
public:
    Building(int id, std::string name, int x, int y, int floors = 1)
        : Location(id, std::move(name), x, y), floors_(floors) {}

    LocationType kind() const override { return LocationType::Building; }

    std::string describe() const override {
        return name() + " [Building, " + std::to_string(floors_) +
               (floors_ == 1 ? " floor]" : " floors]");
    }

    int floors() const { return floors_; }
    void setFloors(int floors) { floors_ = floors; }

private:
    int floors_;
};

// Facility: library, cafeteria, medical centre, parking, sports ground, etc.
class Facility : public Location {
public:
    Facility(int id, std::string name, int x, int y, std::string service)
        : Location(id, std::move(name), x, y), service_(std::move(service)) {}

    LocationType kind() const override { return LocationType::Facility; }

    std::string describe() const override {
        return name() + " [Facility - " + service_ + "]";
    }

    const std::string& service() const { return service_; }
    void setService(const std::string& service) { service_ = service; }

private:
    std::string service_;
};

// Gate: campus entry/exit point; also owns the open/closed state.
class Gate : public Location {
public:
    Gate(int id, std::string name, int x, int y, bool open = true)
        : Location(id, std::move(name), x, y), open_(open) {}

    LocationType kind() const override { return LocationType::Gate; }

    std::string describe() const override {
        return name() + std::string(" [Gate - ") + (open_ ? "OPEN" : "CLOSED") + "]";
    }

    bool isOpen() const { return open_; }
    void setOpen(bool open) { open_ = open; }

private:
    bool open_;
};

}  // namespace campus
