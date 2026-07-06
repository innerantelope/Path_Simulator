#pragma once

#include "smarttraffic/graph/Graph.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace smarttraffic::pathfinding {

enum class Algorithm {
    AStar,
    Dijkstra,
    BFS,
};

struct PathfindingOptions {
    double blockedWeightThreshold{100'000'000.0};
};

struct PathResult {
    std::vector<std::string> path;
    std::vector<std::string> visitedNodes;
    double travelCost{0.0};
    double executionTimeMs{0.0};
    bool success{false};
    std::string algorithm;

    [[nodiscard]] nlohmann::json toJson() const;
};

[[nodiscard]] std::string toString(Algorithm algorithm);
[[nodiscard]] Algorithm algorithmFromString(const std::string& value);

class Pathfinder final {
public:
    [[nodiscard]] static PathResult search(
        const graph::Graph& graph,
        const std::string& start,
        const std::string& goal,
        Algorithm algorithm,
        const PathfindingOptions& options = {});

    [[nodiscard]] static PathResult aStar(
        const graph::Graph& graph,
        const std::string& start,
        const std::string& goal,
        const PathfindingOptions& options = {});

    [[nodiscard]] static PathResult dijkstra(
        const graph::Graph& graph,
        const std::string& start,
        const std::string& goal,
        const PathfindingOptions& options = {});

    [[nodiscard]] static PathResult breadthFirstSearch(
        const graph::Graph& graph,
        const std::string& start,
        const std::string& goal,
        const PathfindingOptions& options = {});

private:
    Pathfinder() = default;
};

}  // namespace smarttraffic::pathfinding
