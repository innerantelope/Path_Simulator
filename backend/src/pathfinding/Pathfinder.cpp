#include "smarttraffic/pathfinding/Pathfinder.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace smarttraffic::pathfinding {
namespace {

using Clock = std::chrono::high_resolution_clock;

double elapsedMilliseconds(Clock::time_point start) {
    const auto elapsed = Clock::now() - start;
    return std::chrono::duration<double, std::milli>(elapsed).count();
}

void validateEndpoints(const graph::Graph& graph, const std::string& start, const std::string& goal) {
    if (!graph.hasNode(start)) {
        throw std::invalid_argument("start node does not exist: " + start);
    }
    if (!graph.hasNode(goal)) {
        throw std::invalid_argument("goal node does not exist: " + goal);
    }
}

double euclideanDistance(const graph::Node& left, const graph::Node& right) {
    const auto dx = left.x() - right.x();
    const auto dy = left.y() - right.y();
    return std::sqrt(dx * dx + dy * dy);
}

std::vector<std::string> reconstructPath(
    const std::unordered_map<std::string, std::string>& previous,
    const std::string& start,
    const std::string& goal) {
    std::vector<std::string> path;
    std::string current = goal;
    path.push_back(current);

    while (current != start) {
        const auto it = previous.find(current);
        if (it == previous.end()) {
            return {};
        }
        current = it->second;
        path.push_back(current);
    }

    std::reverse(path.begin(), path.end());
    return path;
}

double pathCost(const graph::Graph& graph, const std::vector<std::string>& path) {
    if (path.size() < 2) {
        return 0.0;
    }

    double cost = 0.0;
    for (std::size_t index = 1; index < path.size(); ++index) {
        const auto edge = graph.findEdge(path[index - 1], path[index]);
        if (!edge.has_value()) {
            return std::numeric_limits<double>::infinity();
        }
        cost += edge->weight();
    }
    return cost;
}

PathResult weightedSearch(
    const graph::Graph& graph,
    const std::string& start,
    const std::string& goal,
    bool useHeuristic,
    const PathfindingOptions& options) {
    validateEndpoints(graph, start, goal);
    const auto startedAt = Clock::now();

    using QueueEntry = std::pair<double, std::string>;
    std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<>> frontier;
    std::unordered_map<std::string, double> bestCost;
    std::unordered_map<std::string, std::string> previous;
    std::unordered_set<std::string> closed;
    std::vector<std::string> visited;

    bestCost[start] = 0.0;
    frontier.emplace(0.0, start);

    while (!frontier.empty()) {
        const auto current = frontier.top().second;
        frontier.pop();

        if (closed.contains(current)) {
            continue;
        }

        closed.insert(current);
        visited.push_back(current);

        if (current == goal) {
            const auto path = reconstructPath(previous, start, goal);
            return PathResult{
                path,
                visited,
                bestCost.at(goal),
                elapsedMilliseconds(startedAt),
                true,
                useHeuristic ? "A*" : "Dijkstra",
            };
        }

        for (const auto& edge : graph.neighbors(current)) {
            if (edge.weight() >= options.blockedWeightThreshold) {
                continue;
            }

            const auto nextCost = bestCost.at(current) + edge.weight();
            const auto existing = bestCost.find(edge.target());
            if (existing != bestCost.end() && nextCost >= existing->second) {
                continue;
            }

            previous[edge.target()] = current;
            bestCost[edge.target()] = nextCost;

            double priority = nextCost;
            if (useHeuristic) {
                priority += euclideanDistance(graph.node(edge.target()), graph.node(goal));
            }
            frontier.emplace(priority, edge.target());
        }
    }

    return PathResult{{}, visited, 0.0, elapsedMilliseconds(startedAt), false, useHeuristic ? "A*" : "Dijkstra"};
}

}  // namespace

nlohmann::json PathResult::toJson() const {
    return {
        {"path", path},
        {"visitedNodes", visitedNodes},
        {"travelCost", travelCost},
        {"executionTimeMs", executionTimeMs},
        {"success", success},
        {"algorithm", algorithm},
    };
}

std::string toString(Algorithm algorithm) {
    switch (algorithm) {
        case Algorithm::AStar:
            return "A*";
        case Algorithm::Dijkstra:
            return "Dijkstra";
        case Algorithm::BFS:
            return "BFS";
    }
    return "A*";
}

Algorithm algorithmFromString(const std::string& value) {
    if (value == "A*" || value == "astar" || value == "a-star" || value == "AStar") {
        return Algorithm::AStar;
    }
    if (value == "Dijkstra" || value == "dijkstra") {
        return Algorithm::Dijkstra;
    }
    if (value == "BFS" || value == "bfs") {
        return Algorithm::BFS;
    }
    throw std::invalid_argument("unknown pathfinding algorithm: " + value);
}

PathResult Pathfinder::search(
    const graph::Graph& graph,
    const std::string& start,
    const std::string& goal,
    Algorithm algorithm,
    const PathfindingOptions& options) {
    switch (algorithm) {
        case Algorithm::AStar:
            return aStar(graph, start, goal, options);
        case Algorithm::Dijkstra:
            return dijkstra(graph, start, goal, options);
        case Algorithm::BFS:
            return breadthFirstSearch(graph, start, goal, options);
    }
    return aStar(graph, start, goal, options);
}

PathResult Pathfinder::aStar(
    const graph::Graph& graph,
    const std::string& start,
    const std::string& goal,
    const PathfindingOptions& options) {
    return weightedSearch(graph, start, goal, true, options);
}

PathResult Pathfinder::dijkstra(
    const graph::Graph& graph,
    const std::string& start,
    const std::string& goal,
    const PathfindingOptions& options) {
    return weightedSearch(graph, start, goal, false, options);
}

PathResult Pathfinder::breadthFirstSearch(
    const graph::Graph& graph,
    const std::string& start,
    const std::string& goal,
    const PathfindingOptions& options) {
    validateEndpoints(graph, start, goal);
    const auto startedAt = Clock::now();

    std::queue<std::string> frontier;
    std::unordered_set<std::string> seen;
    std::unordered_map<std::string, std::string> previous;
    std::vector<std::string> visited;

    frontier.push(start);
    seen.insert(start);

    while (!frontier.empty()) {
        const auto current = frontier.front();
        frontier.pop();
        visited.push_back(current);

        if (current == goal) {
            const auto path = reconstructPath(previous, start, goal);
            return PathResult{path, visited, pathCost(graph, path), elapsedMilliseconds(startedAt), true, "BFS"};
        }

        for (const auto& edge : graph.neighbors(current)) {
            if (edge.weight() >= options.blockedWeightThreshold || seen.contains(edge.target())) {
                continue;
            }

            previous[edge.target()] = current;
            seen.insert(edge.target());
            frontier.push(edge.target());
        }
    }

    return PathResult{{}, visited, 0.0, elapsedMilliseconds(startedAt), false, "BFS"};
}

}  // namespace smarttraffic::pathfinding
