#include "smarttraffic/simulation/SimulationEngine.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace smarttraffic::simulation {
namespace {

double clamp(double value, double low, double high) {
    return std::max(low, std::min(value, high));
}

double clamp01(double value) {
    return clamp(value, 0.0, 1.0);
}

std::vector<std::string> vehicleEdge(const Vehicle& vehicle) {
    if (vehicle.pathIndex + 1 >= vehicle.currentPath.size()) {
        return {};
    }
    return {vehicle.currentPath[vehicle.pathIndex], vehicle.currentPath[vehicle.pathIndex + 1]};
}

template <typename Map>
nlohmann::json mapValuesToJsonArray(const Map& map) {
    nlohmann::json array = nlohmann::json::array();
    std::vector<typename Map::key_type> keys;
    keys.reserve(map.size());
    for (const auto& entry : map) {
        keys.push_back(entry.first);
    }
    std::sort(keys.begin(), keys.end());
    for (const auto& key : keys) {
        array.push_back(map.at(key).toJson());
    }
    return array;
}

}  // namespace

std::string toString(VehicleType type) {
    switch (type) {
        case VehicleType::Car:
            return "car";
        case VehicleType::Bus:
            return "bus";
        case VehicleType::Police:
            return "police";
        case VehicleType::FireTruck:
            return "fire_truck";
        case VehicleType::Ambulance:
            return "ambulance";
    }
    return "car";
}

VehicleType vehicleTypeFromString(const std::string& value) {
    if (value == "car" || value == "Car") {
        return VehicleType::Car;
    }
    if (value == "bus" || value == "Bus") {
        return VehicleType::Bus;
    }
    if (value == "police" || value == "Police") {
        return VehicleType::Police;
    }
    if (value == "fire_truck" || value == "fire-truck" || value == "fireTruck" || value == "FireTruck") {
        return VehicleType::FireTruck;
    }
    if (value == "ambulance" || value == "Ambulance") {
        return VehicleType::Ambulance;
    }
    throw std::invalid_argument("unknown vehicle type: " + value);
}

bool isEmergencyVehicle(VehicleType type) {
    return type == VehicleType::Police || type == VehicleType::FireTruck || type == VehicleType::Ambulance;
}

double defaultSpeedFor(VehicleType type) {
    switch (type) {
        case VehicleType::Car:
            return 18.0;
        case VehicleType::Bus:
            return 13.5;
        case VehicleType::Police:
            return 24.0;
        case VehicleType::FireTruck:
            return 20.0;
        case VehicleType::Ambulance:
            return 23.0;
    }
    return 18.0;
}

int defaultPriorityFor(VehicleType type) {
    switch (type) {
        case VehicleType::Car:
            return 10;
        case VehicleType::Bus:
            return 20;
        case VehicleType::Police:
            return 90;
        case VehicleType::FireTruck:
            return 95;
        case VehicleType::Ambulance:
            return 100;
    }
    return 10;
}

double defaultFuelFor(VehicleType type) {
    switch (type) {
        case VehicleType::Bus:
            return 180.0;
        case VehicleType::FireTruck:
            return 220.0;
        case VehicleType::Ambulance:
            return 145.0;
        case VehicleType::Police:
            return 130.0;
        case VehicleType::Car:
            return 100.0;
    }
    return 100.0;
}

nlohmann::json Vehicle::toJson() const {
    return {
        {"id", id},
        {"type", toString(type)},
        {"emergency", isEmergencyVehicle(type)},
        {"currentNode", currentNode},
        {"destinationNode", destinationNode},
        {"speed", speed},
        {"currentPath", currentPath},
        {"pathIndex", pathIndex},
        {"edgeProgress", edgeProgress},
        {"fuel", fuel},
        {"waitingTime", waitingTime},
        {"priority", priority},
        {"position", {{"x", positionX}, {"y", positionY}}},
        {"routeCost", routeCost},
        {"completedTrips", completedTrips},
        {"rerouteCount", rerouteCount},
    };
}

std::string toString(TrafficLightState state) {
    switch (state) {
        case TrafficLightState::Green:
            return "green";
        case TrafficLightState::Yellow:
            return "yellow";
        case TrafficLightState::Red:
            return "red";
    }
    return "green";
}

TrafficLight::TrafficLight(std::string nodeId) : nodeId_(std::move(nodeId)) {}

const std::string& TrafficLight::nodeId() const noexcept {
    return nodeId_;
}

TrafficLightState TrafficLight::state() const noexcept {
    return state_;
}

double TrafficLight::timer() const noexcept {
    return timer_;
}

void TrafficLight::update(double deltaSeconds) {
    if (priorityHoldSeconds_ > 0.0) {
        priorityHoldSeconds_ = std::max(0.0, priorityHoldSeconds_ - deltaSeconds);
        state_ = TrafficLightState::Green;
        timer_ = 0.0;
        return;
    }

    timer_ += deltaSeconds;
    switch (state_) {
        case TrafficLightState::Green:
            if (timer_ >= greenSeconds_) {
                state_ = TrafficLightState::Yellow;
                timer_ = 0.0;
            }
            break;
        case TrafficLightState::Yellow:
            if (timer_ >= yellowSeconds_) {
                state_ = TrafficLightState::Red;
                timer_ = 0.0;
            }
            break;
        case TrafficLightState::Red:
            if (timer_ >= redSeconds_) {
                state_ = TrafficLightState::Green;
                timer_ = 0.0;
            }
            break;
    }
}

void TrafficLight::forceGreen(double seconds) {
    state_ = TrafficLightState::Green;
    timer_ = 0.0;
    priorityHoldSeconds_ = std::max(priorityHoldSeconds_, seconds);
}

nlohmann::json TrafficLight::toJson() const {
    return {
        {"nodeId", nodeId_},
        {"state", toString(state_)},
        {"timer", timer_},
        {"priorityHoldSeconds", priorityHoldSeconds_},
    };
}

std::string toString(AccidentKind kind) {
    switch (kind) {
        case AccidentKind::RoadClosure:
            return "road_closure";
        case AccidentKind::PartialBlockage:
            return "partial_blockage";
        case AccidentKind::HeavyCongestion:
            return "heavy_congestion";
    }
    return "partial_blockage";
}

AccidentKind accidentKindFromString(const std::string& value) {
    if (value == "road_closure" || value == "road-closure" || value == "closure") {
        return AccidentKind::RoadClosure;
    }
    if (value == "partial_blockage" || value == "partial-blockage" || value == "blockage") {
        return AccidentKind::PartialBlockage;
    }
    if (value == "heavy_congestion" || value == "heavy-congestion" || value == "congestion") {
        return AccidentKind::HeavyCongestion;
    }
    throw std::invalid_argument("unknown accident kind: " + value);
}

nlohmann::json RoadState::toJson() const {
    return {
        {"source", source},
        {"target", target},
        {"baseWeight", baseWeight},
        {"currentWeight", currentWeight},
        {"vehicleCount", vehicleCount},
        {"averageSpeed", averageSpeed},
        {"congestionScore", congestionScore},
        {"travelTime", travelTime},
        {"closed", closed},
        {"accident", accident},
        {"accidentSeverity", accidentSeverity},
    };
}

nlohmann::json Accident::toJson() const {
    return {
        {"id", id},
        {"source", source},
        {"target", target},
        {"kind", toString(kind)},
        {"severity", severity},
        {"active", active},
    };
}

nlohmann::json SimulationStats::toJson() const {
    return {
        {"simulatedTimeSeconds", simulatedTimeSeconds},
        {"averageTravelTime", averageTravelTime},
        {"averageCongestion", averageCongestion},
        {"vehicleThroughput", vehicleThroughput},
        {"fuelConsumption", fuelConsumption},
        {"waitingTime", waitingTime},
        {"roadUtilization", roadUtilization},
        {"activeVehicles", activeVehicles},
        {"accidentCount", accidentCount},
        {"closedRoadCount", closedRoadCount},
        {"collisionWarnings", collisionWarnings},
        {"tick", tick},
    };
}

SimulationEngine::SimulationEngine() {
    generateCity(city::CityGenerationConfig{city::CityLayout::Grid, 64, 0.12, 80.0, false, 42});
    const auto spawnedIds = spawnVehicles(40, VehicleType::Car);
    static_cast<void>(spawnedIds);
}

void SimulationEngine::loadCity(graph::Graph graph) {
    std::lock_guard lock(mutex_);
    graph_ = std::move(graph);
    initialGraph_ = graph_;
    vehicles_.clear();
    accidents_.clear();
    stats_ = SimulationStats{};
    running_ = false;
    nextVehicleNumber_ = 1;
    nextAccidentNumber_ = 1;
    initializeRoadStates();
    initializeTrafficLights();
}

void SimulationEngine::generateCity(const city::CityGenerationConfig& config) {
    loadCity(city::CityGenerator::generate(config));
}

graph::Graph SimulationEngine::cityGraph() const {
    std::lock_guard lock(mutex_);
    return graph_;
}

std::string SimulationEngine::spawnVehicle(const SpawnVehicleRequest& request) {
    std::lock_guard lock(mutex_);
    if (graph_.nodeCount() < 2) {
        throw std::runtime_error("cannot spawn vehicles without at least two nodes");
    }

    Vehicle vehicle;
    vehicle.id = nextVehicleId();
    vehicle.type = request.type;
    vehicle.speed = request.speed > 0.0 ? request.speed : defaultSpeedFor(vehicle.type);
    vehicle.priority = defaultPriorityFor(vehicle.type);
    vehicle.fuel = defaultFuelFor(vehicle.type);
    vehicle.currentNode = request.source.empty() ? randomNodeExcept("") : request.source;
    vehicle.destinationNode = request.destination.empty() ? randomNodeExcept(vehicle.currentNode) : request.destination;

    if (!graph_.hasNode(vehicle.currentNode)) {
        throw std::invalid_argument("vehicle source node does not exist: " + vehicle.currentNode);
    }
    if (!graph_.hasNode(vehicle.destinationNode)) {
        throw std::invalid_argument("vehicle destination node does not exist: " + vehicle.destinationNode);
    }
    if (vehicle.currentNode == vehicle.destinationNode) {
        vehicle.destinationNode = randomNodeExcept(vehicle.currentNode);
    }

    const auto& current = graph_.node(vehicle.currentNode);
    vehicle.positionX = current.x();
    vehicle.positionY = current.y();
    updateVehicleRoute(vehicle, pathfinding::Algorithm::AStar);
    vehicles_.emplace(vehicle.id, vehicle);
    return vehicle.id;
}

std::vector<std::string> SimulationEngine::spawnVehicles(std::size_t count, VehicleType type) {
    std::vector<std::string> ids;
    ids.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        ids.push_back(spawnVehicle(SpawnVehicleRequest{type, "", "", 0.0}));
    }
    return ids;
}

bool SimulationEngine::removeVehicle(const std::string& id) {
    std::lock_guard lock(mutex_);
    return vehicles_.erase(id) > 0;
}

void SimulationEngine::start() {
    std::lock_guard lock(mutex_);
    running_ = true;
}

void SimulationEngine::pause() {
    std::lock_guard lock(mutex_);
    running_ = false;
}

void SimulationEngine::resume() {
    start();
}

void SimulationEngine::reset() {
    std::lock_guard lock(mutex_);
    graph_ = initialGraph_;
    vehicles_.clear();
    accidents_.clear();
    roadStates_.clear();
    trafficLights_.clear();
    stats_ = SimulationStats{};
    running_ = false;
    speedMultiplier_ = 1.0;
    nextVehicleNumber_ = 1;
    nextAccidentNumber_ = 1;
    initializeRoadStates();
    initializeTrafficLights();
}

void SimulationEngine::setSpeedMultiplier(double multiplier) {
    if (!std::isfinite(multiplier) || multiplier <= 0.0 || multiplier > 10.0) {
        throw std::invalid_argument("simulation speed multiplier must be between 0 and 10");
    }
    std::lock_guard lock(mutex_);
    speedMultiplier_ = multiplier;
}

bool SimulationEngine::running() const {
    std::lock_guard lock(mutex_);
    return running_;
}

double SimulationEngine::speedMultiplier() const {
    std::lock_guard lock(mutex_);
    return speedMultiplier_;
}

void SimulationEngine::tick(double deltaSeconds) {
    if (!std::isfinite(deltaSeconds) || deltaSeconds < 0.0) {
        throw std::invalid_argument("delta seconds must be a finite non-negative value");
    }

    std::lock_guard lock(mutex_);
    if (!running_) {
        refreshStats();
        return;
    }

    const auto scaledDelta = deltaSeconds * speedMultiplier_;
    stats_.simulatedTimeSeconds += scaledDelta;
    ++stats_.tick;

    updateRoadCosts();
    updateTrafficLights(scaledDelta);
    updateVehicles(scaledDelta);
    maybeGenerateAccident(scaledDelta);
    updateRoadCosts();
    detectCollisionWarnings();
    refreshStats();
}

void SimulationEngine::closeRoad(const std::string& source, const std::string& target) {
    std::lock_guard lock(mutex_);
    const auto state = roadStateFor(source, target);
    if (!state.has_value()) {
        throw std::invalid_argument("road does not exist: " + source + " -> " + target);
    }

    state->get().closed = true;
    graph_.updateEdgeWeight(source, target, kClosedRoadWeight);
}

void SimulationEngine::openRoad(const std::string& source, const std::string& target) {
    std::lock_guard lock(mutex_);
    const auto state = roadStateFor(source, target);
    if (!state.has_value()) {
        throw std::invalid_argument("road does not exist: " + source + " -> " + target);
    }

    state->get().closed = false;
    state->get().accident = false;
    state->get().accidentSeverity = 0.0;
    graph_.updateEdgeWeight(source, target, state->get().baseWeight);
}

Accident SimulationEngine::createAccident(
    const std::string& source,
    const std::string& target,
    AccidentKind kind,
    double severity) {
    if (!std::isfinite(severity)) {
        throw std::invalid_argument("accident severity must be finite");
    }

    std::lock_guard lock(mutex_);
    const auto state = roadStateFor(source, target);
    if (!state.has_value()) {
        throw std::invalid_argument("road does not exist: " + source + " -> " + target);
    }

    Accident accident{nextAccidentId(), source, target, kind, clamp01(severity), true};
    accidents_.emplace(accident.id, accident);

    auto& road = state->get();
    road.accident = true;
    road.accidentSeverity = accident.severity;
    if (kind == AccidentKind::RoadClosure) {
        road.closed = true;
    }

    updateRoadCosts();
    return accident;
}

bool SimulationEngine::deleteAccident(const std::string& id) {
    std::lock_guard lock(mutex_);
    const auto accidentIt = accidents_.find(id);
    if (accidentIt == accidents_.end()) {
        return false;
    }

    const auto accident = accidentIt->second;
    accidents_.erase(accidentIt);
    const auto state = roadStateFor(accident.source, accident.target);
    if (state.has_value()) {
        state->get().accident = false;
        state->get().accidentSeverity = 0.0;
        if (accident.kind == AccidentKind::RoadClosure) {
            state->get().closed = false;
        }
    }
    updateRoadCosts();
    return true;
}

void SimulationEngine::clearAccidents() {
    std::lock_guard lock(mutex_);
    accidents_.clear();
    for (auto& entry : roadStates_) {
        entry.second.accident = false;
        entry.second.accidentSeverity = 0.0;
    }
    updateRoadCosts();
}

pathfinding::PathResult SimulationEngine::route(
    const std::string& source,
    const std::string& destination,
    pathfinding::Algorithm algorithm) const {
    std::lock_guard lock(mutex_);
    return routeUnlocked(source, destination, algorithm);
}

nlohmann::json SimulationEngine::cityJson() const {
    std::lock_guard lock(mutex_);
    return {
        {"graph", graph_.toJson()},
        {"roads", mapValuesToJsonArray(roadStates_)},
        {"trafficLights", mapValuesToJsonArray(trafficLights_)},
    };
}

nlohmann::json SimulationEngine::vehiclesJson() const {
    std::lock_guard lock(mutex_);
    return mapValuesToJsonArray(vehicles_);
}

nlohmann::json SimulationEngine::roadStatesJson() const {
    std::lock_guard lock(mutex_);
    return mapValuesToJsonArray(roadStates_);
}

nlohmann::json SimulationEngine::trafficLightsJson() const {
    std::lock_guard lock(mutex_);
    return mapValuesToJsonArray(trafficLights_);
}

nlohmann::json SimulationEngine::accidentsJson() const {
    std::lock_guard lock(mutex_);
    return mapValuesToJsonArray(accidents_);
}

nlohmann::json SimulationEngine::statsJson() const {
    std::lock_guard lock(mutex_);
    return stats_.toJson();
}

nlohmann::json SimulationEngine::simulationJson() const {
    std::lock_guard lock(mutex_);
    return {
        {"running", running_},
        {"speedMultiplier", speedMultiplier_},
        {"stats", stats_.toJson()},
        {"vehicleCount", vehicles_.size()},
        {"roadCount", roadStates_.size()},
    };
}

nlohmann::json SimulationEngine::snapshotJson() const {
    std::lock_guard lock(mutex_);
    return {
        {"type", "simulation_update"},
        {"simulation", {
            {"running", running_},
            {"speedMultiplier", speedMultiplier_},
            {"tick", stats_.tick},
            {"simulatedTimeSeconds", stats_.simulatedTimeSeconds},
        }},
        {"vehicles", mapValuesToJsonArray(vehicles_)},
        {"trafficLights", mapValuesToJsonArray(trafficLights_)},
        {"roads", mapValuesToJsonArray(roadStates_)},
        {"accidents", mapValuesToJsonArray(accidents_)},
        {"stats", stats_.toJson()},
    };
}

std::string SimulationEngine::nextVehicleId() {
    return "veh-" + std::to_string(nextVehicleNumber_++);
}

std::string SimulationEngine::nextAccidentId() {
    return "acc-" + std::to_string(nextAccidentNumber_++);
}

std::string SimulationEngine::roadKey(const std::string& source, const std::string& target) const {
    if (graph_.directed() || source <= target) {
        return source + "->" + target;
    }
    return target + "->" + source;
}

std::string SimulationEngine::randomNodeExcept(const std::string& excluded) {
    const auto nodes = graph_.nodes();
    if (nodes.empty()) {
        throw std::runtime_error("city has no nodes");
    }

    std::uniform_int_distribution<std::size_t> distribution(0, nodes.size() - 1);
    for (std::size_t attempt = 0; attempt < nodes.size() * 2; ++attempt) {
        const auto candidate = nodes[distribution(rng_)].id();
        if (candidate != excluded) {
            return candidate;
        }
    }

    for (const auto& node : nodes) {
        if (node.id() != excluded) {
            return node.id();
        }
    }
    return nodes.front().id();
}

pathfinding::PathResult SimulationEngine::routeUnlocked(
    const std::string& source,
    const std::string& destination,
    pathfinding::Algorithm algorithm) const {
    return pathfinding::Pathfinder::search(
        graph_,
        source,
        destination,
        algorithm,
        pathfinding::PathfindingOptions{100'000'000.0});
}

std::optional<std::reference_wrapper<RoadState>> SimulationEngine::roadStateFor(
    const std::string& source,
    const std::string& target) {
    const auto it = roadStates_.find(roadKey(source, target));
    if (it == roadStates_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<std::reference_wrapper<const RoadState>> SimulationEngine::roadStateFor(
    const std::string& source,
    const std::string& target) const {
    const auto it = roadStates_.find(roadKey(source, target));
    if (it == roadStates_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void SimulationEngine::initializeRoadStates() {
    roadStates_.clear();
    for (const auto& edge : graph_.edges()) {
        const auto key = roadKey(edge.source(), edge.target());
        roadStates_.emplace(
            key,
            RoadState{
                edge.source(),
                edge.target(),
                edge.weight(),
                edge.weight(),
                0,
                defaultSpeedFor(VehicleType::Car),
                0.0,
                edge.weight() / defaultSpeedFor(VehicleType::Car),
                false,
                false,
                0.0,
            });
    }
}

void SimulationEngine::initializeTrafficLights() {
    trafficLights_.clear();
    for (const auto& node : graph_.nodes()) {
        trafficLights_.emplace(node.id(), TrafficLight{node.id()});
    }
}

void SimulationEngine::updateRoadCosts() {
    for (auto& entry : roadStates_) {
        auto& road = entry.second;
        road.vehicleCount = 0;
        road.averageSpeed = defaultSpeedFor(VehicleType::Car);
        road.congestionScore = 0.0;
    }

    for (const auto& vehicleEntry : vehicles_) {
        const auto edge = vehicleEdge(vehicleEntry.second);
        if (edge.size() != 2) {
            continue;
        }
        const auto state = roadStateFor(edge[0], edge[1]);
        if (state.has_value()) {
            ++state->get().vehicleCount;
        }
    }

    for (auto& entry : roadStates_) {
        auto& road = entry.second;
        road.congestionScore = clamp01(static_cast<double>(road.vehicleCount) / 12.0);

        const auto congestionPenalty = 1.0 + road.congestionScore * 3.0;
        const auto accidentPenalty = road.accident ? 1.0 + road.accidentSeverity * 6.0 : 1.0;
        road.currentWeight = road.closed ? kClosedRoadWeight : road.baseWeight * congestionPenalty * accidentPenalty;
        road.averageSpeed = road.closed
            ? 0.0
            : defaultSpeedFor(VehicleType::Car) * clamp(1.0 - road.congestionScore * 0.65, 0.2, 1.0) /
                  (road.accident ? 1.0 + road.accidentSeverity * 2.0 : 1.0);
        road.travelTime = road.closed ? kClosedRoadWeight : road.baseWeight / std::max(1.0, road.averageSpeed);
        graph_.updateEdgeWeight(road.source, road.target, road.currentWeight);
    }
}

void SimulationEngine::updateTrafficLights(double deltaSeconds) {
    for (auto& entry : trafficLights_) {
        entry.second.update(deltaSeconds);
    }
}

void SimulationEngine::updateVehicles(double deltaSeconds) {
    for (auto& entry : vehicles_) {
        auto& vehicle = entry.second;

        if (vehicle.fuel <= 0.0) {
            vehicle.waitingTime += deltaSeconds;
            stats_.waitingTime += deltaSeconds;
            continue;
        }

        if (vehicle.currentPath.empty() || vehicle.pathIndex + 1 >= vehicle.currentPath.size()) {
            if (vehicle.currentNode == vehicle.destinationNode) {
                assignNewDestination(vehicle);
            } else {
                updateVehicleRoute(vehicle, pathfinding::Algorithm::AStar);
            }

            if (vehicle.currentPath.empty() || vehicle.pathIndex + 1 >= vehicle.currentPath.size()) {
                vehicle.waitingTime += deltaSeconds;
                stats_.waitingTime += deltaSeconds;
                continue;
            }
        }

        const auto nextNode = vehicle.currentPath[vehicle.pathIndex + 1];
        auto road = roadStateFor(vehicle.currentNode, nextNode);
        if (!road.has_value() || road->get().closed) {
            updateVehicleRoute(vehicle, pathfinding::Algorithm::AStar);
            vehicle.waitingTime += deltaSeconds;
            stats_.waitingTime += deltaSeconds;
            continue;
        }

        const auto lightIt = trafficLights_.find(nextNode);
        if (lightIt != trafficLights_.end()) {
            if (isEmergencyVehicle(vehicle.type)) {
                lightIt->second.forceGreen(4.0);
            } else if (vehicle.edgeProgress > 0.75 && lightIt->second.state() != TrafficLightState::Green) {
                vehicle.waitingTime += deltaSeconds;
                stats_.waitingTime += deltaSeconds;
                continue;
            }
        }

        const auto congestionFactor = clamp(1.0 - road->get().congestionScore * 0.7, 0.2, 1.0);
        const auto accidentFactor = road->get().accident ? clamp(1.0 - road->get().accidentSeverity * 0.6, 0.1, 1.0) : 1.0;
        const auto distance = std::max(1.0, road->get().baseWeight);
        vehicle.edgeProgress += (vehicle.speed * congestionFactor * accidentFactor * deltaSeconds) / distance;

        const auto fuelUsed = deltaSeconds * (0.025 + vehicle.speed * 0.0005 + road->get().congestionScore * 0.04);
        vehicle.fuel = std::max(0.0, vehicle.fuel - fuelUsed);
        stats_.fuelConsumption += fuelUsed;

        if (vehicle.edgeProgress >= 1.0) {
            vehicle.currentNode = nextNode;
            vehicle.edgeProgress = 0.0;
            ++vehicle.pathIndex;
            updateVehiclePosition(vehicle);

            if (vehicle.currentNode == vehicle.destinationNode) {
                ++vehicle.completedTrips;
                stats_.vehicleThroughput += 1.0;
                assignNewDestination(vehicle);
            }
        } else {
            updateVehiclePosition(vehicle);
        }
    }
}

void SimulationEngine::updateVehicleRoute(Vehicle& vehicle, pathfinding::Algorithm algorithm) {
    const auto result = routeUnlocked(vehicle.currentNode, vehicle.destinationNode, algorithm);
    if (!result.success || result.path.empty()) {
        vehicle.currentPath.clear();
        vehicle.pathIndex = 0;
        vehicle.routeCost = 0.0;
        return;
    }

    vehicle.currentPath = result.path;
    vehicle.pathIndex = 0;
    vehicle.edgeProgress = 0.0;
    vehicle.routeCost = result.travelCost;
    ++vehicle.rerouteCount;
}

void SimulationEngine::assignNewDestination(Vehicle& vehicle) {
    vehicle.destinationNode = randomNodeExcept(vehicle.currentNode);
    updateVehicleRoute(vehicle, pathfinding::Algorithm::AStar);
}

void SimulationEngine::updateVehiclePosition(Vehicle& vehicle) const {
    if (vehicle.pathIndex + 1 >= vehicle.currentPath.size()) {
        const auto& node = graph_.node(vehicle.currentNode);
        vehicle.positionX = node.x();
        vehicle.positionY = node.y();
        return;
    }

    const auto& start = graph_.node(vehicle.currentPath[vehicle.pathIndex]);
    const auto& end = graph_.node(vehicle.currentPath[vehicle.pathIndex + 1]);
    const auto progress = clamp01(vehicle.edgeProgress);
    vehicle.positionX = start.x() + (end.x() - start.x()) * progress;
    vehicle.positionY = start.y() + (end.y() - start.y()) * progress;
}

void SimulationEngine::maybeGenerateAccident(double deltaSeconds) {
    if (roadStates_.empty() || accidents_.size() >= 12) {
        return;
    }

    std::uniform_real_distribution<double> chance(0.0, 1.0);
    if (chance(rng_) > 0.0008 * deltaSeconds) {
        return;
    }

    std::vector<std::string> availableRoads;
    availableRoads.reserve(roadStates_.size());
    for (const auto& entry : roadStates_) {
        if (!entry.second.closed && !entry.second.accident) {
            availableRoads.push_back(entry.first);
        }
    }
    if (availableRoads.empty()) {
        return;
    }

    std::uniform_int_distribution<std::size_t> roadDistribution(0, availableRoads.size() - 1);
    std::uniform_real_distribution<double> severityDistribution(0.35, 0.9);
    auto& road = roadStates_.at(availableRoads[roadDistribution(rng_)]);
    Accident accident{
        nextAccidentId(),
        road.source,
        road.target,
        AccidentKind::HeavyCongestion,
        severityDistribution(rng_),
        true,
    };
    road.accident = true;
    road.accidentSeverity = accident.severity;
    accidents_.emplace(accident.id, accident);
}

void SimulationEngine::detectCollisionWarnings() {
    stats_.collisionWarnings = 0;
    std::vector<const Vehicle*> activeVehicles;
    activeVehicles.reserve(vehicles_.size());
    for (const auto& entry : vehicles_) {
        activeVehicles.push_back(&entry.second);
    }

    for (std::size_t i = 0; i < activeVehicles.size(); ++i) {
        const auto firstEdge = vehicleEdge(*activeVehicles[i]);
        if (firstEdge.size() != 2) {
            continue;
        }

        for (std::size_t j = i + 1; j < activeVehicles.size(); ++j) {
            const auto secondEdge = vehicleEdge(*activeVehicles[j]);
            if (secondEdge == firstEdge && std::abs(activeVehicles[i]->edgeProgress - activeVehicles[j]->edgeProgress) < 0.015) {
                ++stats_.collisionWarnings;
            }
        }
    }
}

void SimulationEngine::refreshStats() {
    stats_.activeVehicles = vehicles_.size();
    stats_.accidentCount = accidents_.size();

    double congestion = 0.0;
    std::size_t utilizedRoads = 0;
    stats_.closedRoadCount = 0;
    for (const auto& entry : roadStates_) {
        congestion += entry.second.congestionScore;
        if (entry.second.vehicleCount > 0) {
            ++utilizedRoads;
        }
        if (entry.second.closed) {
            ++stats_.closedRoadCount;
        }
    }

    if (!roadStates_.empty()) {
        stats_.averageCongestion = congestion / static_cast<double>(roadStates_.size());
        stats_.roadUtilization = static_cast<double>(utilizedRoads) / static_cast<double>(roadStates_.size());
    } else {
        stats_.averageCongestion = 0.0;
        stats_.roadUtilization = 0.0;
    }

    stats_.averageTravelTime = stats_.vehicleThroughput > 0.0
        ? stats_.simulatedTimeSeconds / stats_.vehicleThroughput
        : 0.0;
}

}  // namespace smarttraffic::simulation
