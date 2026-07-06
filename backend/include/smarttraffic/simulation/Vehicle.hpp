#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace smarttraffic::simulation {

enum class VehicleType {
    Car,
    Bus,
    Police,
    FireTruck,
    Ambulance,
};

[[nodiscard]] std::string toString(VehicleType type);
[[nodiscard]] VehicleType vehicleTypeFromString(const std::string& value);
[[nodiscard]] bool isEmergencyVehicle(VehicleType type);
[[nodiscard]] double defaultSpeedFor(VehicleType type);
[[nodiscard]] int defaultPriorityFor(VehicleType type);
[[nodiscard]] double defaultFuelFor(VehicleType type);

struct Vehicle {
    std::string id;
    VehicleType type{VehicleType::Car};
    std::string currentNode;
    std::string destinationNode;
    double speed{18.0};
    std::vector<std::string> currentPath;
    std::size_t pathIndex{0};
    double edgeProgress{0.0};
    double fuel{100.0};
    double waitingTime{0.0};
    int priority{10};
    double positionX{0.0};
    double positionY{0.0};
    double routeCost{0.0};
    std::size_t completedTrips{0};
    std::size_t rerouteCount{0};

    [[nodiscard]] nlohmann::json toJson() const;
};

}  // namespace smarttraffic::simulation
