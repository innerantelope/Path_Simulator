#pragma once

#include "smarttraffic/city/CityGenerator.hpp"
#include "smarttraffic/pathfinding/Pathfinder.hpp"
#include "smarttraffic/simulation/TrafficLight.hpp"
#include "smarttraffic/simulation/Vehicle.hpp"

#include <nlohmann/json.hpp>

#include <functional>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace smarttraffic::simulation {

enum class AccidentKind {
    RoadClosure,
    PartialBlockage,
    HeavyCongestion,
};

[[nodiscard]] std::string toString(AccidentKind kind);
[[nodiscard]] AccidentKind accidentKindFromString(const std::string& value);

struct RoadState {
    std::string source;
    std::string target;
    double baseWeight{1.0};
    double currentWeight{1.0};
    std::size_t vehicleCount{0};
    double averageSpeed{0.0};
    double congestionScore{0.0};
    double travelTime{0.0};
    bool closed{false};
    bool accident{false};
    double accidentSeverity{0.0};

    [[nodiscard]] nlohmann::json toJson() const;
};

struct Accident {
    std::string id;
    std::string source;
    std::string target;
    AccidentKind kind{AccidentKind::PartialBlockage};
    double severity{0.65};
    bool active{true};

    [[nodiscard]] nlohmann::json toJson() const;
};

struct SimulationStats {
    double simulatedTimeSeconds{0.0};
    double averageTravelTime{0.0};
    double averageCongestion{0.0};
    double vehicleThroughput{0.0};
    double fuelConsumption{0.0};
    double waitingTime{0.0};
    double roadUtilization{0.0};
    std::size_t activeVehicles{0};
    std::size_t accidentCount{0};
    std::size_t closedRoadCount{0};
    std::size_t collisionWarnings{0};
    std::size_t tick{0};

    [[nodiscard]] nlohmann::json toJson() const;
};

struct SpawnVehicleRequest {
    VehicleType type{VehicleType::Car};
    std::string source;
    std::string destination;
    double speed{0.0};
};

class SimulationEngine final {
public:
    SimulationEngine();

    void loadCity(graph::Graph graph);
    void generateCity(const city::CityGenerationConfig& config);
    [[nodiscard]] graph::Graph cityGraph() const;

    [[nodiscard]] std::string spawnVehicle(const SpawnVehicleRequest& request);
    [[nodiscard]] std::vector<std::string> spawnVehicles(std::size_t count, VehicleType type);
    bool removeVehicle(const std::string& id);

    void start();
    void pause();
    void resume();
    void reset();
    void setSpeedMultiplier(double multiplier);
    [[nodiscard]] bool running() const;
    [[nodiscard]] double speedMultiplier() const;

    void tick(double deltaSeconds);

    void closeRoad(const std::string& source, const std::string& target);
    void openRoad(const std::string& source, const std::string& target);
    [[nodiscard]] Accident createAccident(
        const std::string& source,
        const std::string& target,
        AccidentKind kind,
        double severity);
    bool deleteAccident(const std::string& id);
    void clearAccidents();

    [[nodiscard]] pathfinding::PathResult route(
        const std::string& source,
        const std::string& destination,
        pathfinding::Algorithm algorithm) const;

    [[nodiscard]] nlohmann::json cityJson() const;
    [[nodiscard]] nlohmann::json vehiclesJson() const;
    [[nodiscard]] nlohmann::json roadStatesJson() const;
    [[nodiscard]] nlohmann::json trafficLightsJson() const;
    [[nodiscard]] nlohmann::json accidentsJson() const;
    [[nodiscard]] nlohmann::json statsJson() const;
    [[nodiscard]] nlohmann::json simulationJson() const;
    [[nodiscard]] nlohmann::json snapshotJson() const;

private:
    static constexpr double kClosedRoadWeight = 1'000'000'000.0;

    [[nodiscard]] std::string nextVehicleId();
    [[nodiscard]] std::string nextAccidentId();
    [[nodiscard]] std::string roadKey(const std::string& source, const std::string& target) const;
    [[nodiscard]] std::string randomNodeExcept(const std::string& excluded);
    [[nodiscard]] pathfinding::PathResult routeUnlocked(
        const std::string& source,
        const std::string& destination,
        pathfinding::Algorithm algorithm) const;
    [[nodiscard]] std::optional<std::reference_wrapper<RoadState>> roadStateFor(
        const std::string& source,
        const std::string& target);
    [[nodiscard]] std::optional<std::reference_wrapper<const RoadState>> roadStateFor(
        const std::string& source,
        const std::string& target) const;

    void initializeRoadStates();
    void initializeTrafficLights();
    void updateRoadCosts();
    void updateTrafficLights(double deltaSeconds);
    void updateVehicles(double deltaSeconds);
    void updateVehicleRoute(Vehicle& vehicle, pathfinding::Algorithm algorithm);
    void assignNewDestination(Vehicle& vehicle);
    void updateVehiclePosition(Vehicle& vehicle) const;
    void maybeGenerateAccident(double deltaSeconds);
    void detectCollisionWarnings();
    void refreshStats();

    mutable std::mutex mutex_;
    graph::Graph graph_{false};
    graph::Graph initialGraph_{false};
    std::unordered_map<std::string, RoadState> roadStates_;
    std::unordered_map<std::string, Vehicle> vehicles_;
    std::unordered_map<std::string, TrafficLight> trafficLights_;
    std::unordered_map<std::string, Accident> accidents_;
    SimulationStats stats_;
    bool running_{false};
    double speedMultiplier_{1.0};
    std::size_t nextVehicleNumber_{1};
    std::size_t nextAccidentNumber_{1};
    mutable std::mt19937 rng_{42};
};

}  // namespace smarttraffic::simulation
