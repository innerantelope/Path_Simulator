#include "smarttraffic/city/CityGenerator.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <stdexcept>
#include <utility>

namespace smarttraffic::city {
namespace {

constexpr double kPi = 3.14159265358979323846;

void validateConfig(const CityGenerationConfig& config) {
    if (config.intersections < 2) {
        throw std::invalid_argument("city generation requires at least two intersections");
    }
    if (!std::isfinite(config.roadDensity) || config.roadDensity < 0.0 || config.roadDensity > 1.0) {
        throw std::invalid_argument("road density must be between 0 and 1");
    }
    if (!std::isfinite(config.averageRoadLength) || config.averageRoadLength <= 0.0) {
        throw std::invalid_argument("average road length must be positive");
    }
}

double distanceBetween(const graph::Node& left, const graph::Node& right) {
    const auto dx = left.x() - right.x();
    const auto dy = left.y() - right.y();
    return std::sqrt(dx * dx + dy * dy);
}

double distanceBetween(const graph::Graph& graph, const std::string& left, const std::string& right) {
    return distanceBetween(graph.node(left), graph.node(right));
}

std::string nodeId(std::size_t index) {
    return "N" + std::to_string(index);
}

void addRoadIfMissing(graph::Graph& graph, const std::string& source, const std::string& target) {
    if (source == target || graph.hasEdge(source, target)) {
        return;
    }

    graph.addEdge(source, target, distanceBetween(graph, source, target), {{"baseWeight", distanceBetween(graph, source, target)}});
}

std::vector<std::string> nodeIds(const graph::Graph& graph) {
    std::vector<std::string> ids;
    ids.reserve(graph.nodeCount());
    for (const auto& node : graph.nodes()) {
        ids.push_back(node.id());
    }
    return ids;
}

void addDensityRoads(graph::Graph& graph, double density, std::mt19937& rng) {
    const auto ids = nodeIds(graph);
    if (ids.size() < 2) {
        return;
    }

    const auto maxEdges = graph.directed()
        ? ids.size() * (ids.size() - 1)
        : (ids.size() * (ids.size() - 1)) / 2;
    const auto targetEdges = static_cast<std::size_t>(std::round(static_cast<double>(maxEdges) * density));

    if (graph.edgeCount() >= targetEdges) {
        return;
    }

    std::vector<std::pair<std::string, std::string>> candidates;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        for (std::size_t j = 0; j < ids.size(); ++j) {
            if (i == j) {
                continue;
            }
            if (!graph.directed() && j <= i) {
                continue;
            }
            if (!graph.hasEdge(ids[i], ids[j])) {
                candidates.emplace_back(ids[i], ids[j]);
            }
        }
    }

    std::shuffle(candidates.begin(), candidates.end(), rng);
    for (const auto& [source, target] : candidates) {
        if (graph.edgeCount() >= targetEdges) {
            break;
        }
        addRoadIfMissing(graph, source, target);
    }
}

graph::Graph generateGrid(const CityGenerationConfig& config, std::mt19937& rng) {
    graph::Graph graph{config.directed};
    const auto columns = static_cast<std::size_t>(std::ceil(std::sqrt(static_cast<double>(config.intersections))));
    const auto rows = static_cast<std::size_t>(std::ceil(static_cast<double>(config.intersections) / static_cast<double>(columns)));

    for (std::size_t index = 0; index < config.intersections; ++index) {
        const auto row = index / columns;
        const auto column = index % columns;
        graph.addNode(
            nodeId(index),
            static_cast<double>(column) * config.averageRoadLength,
            static_cast<double>(row) * config.averageRoadLength,
            {{"layout", "grid"}, {"row", row}, {"column", column}});
    }

    for (std::size_t index = 0; index < config.intersections; ++index) {
        const auto row = index / columns;
        const auto column = index % columns;
        const auto right = index + 1;
        const auto down = index + columns;

        if (column + 1 < columns && right < config.intersections) {
            addRoadIfMissing(graph, nodeId(index), nodeId(right));
        }
        if (row + 1 < rows && down < config.intersections) {
            addRoadIfMissing(graph, nodeId(index), nodeId(down));
        }
    }

    addDensityRoads(graph, config.roadDensity, rng);
    return graph;
}

graph::Graph generateRandomCity(const CityGenerationConfig& config, std::mt19937& rng) {
    graph::Graph graph{config.directed};
    const auto sideLength = std::sqrt(static_cast<double>(config.intersections)) * config.averageRoadLength;
    std::uniform_real_distribution<double> coordinate(0.0, sideLength);

    for (std::size_t index = 0; index < config.intersections; ++index) {
        graph.addNode(nodeId(index), coordinate(rng), coordinate(rng), {{"layout", "random"}});
    }

    const auto ids = nodeIds(graph);
    for (const auto& source : ids) {
        std::vector<std::pair<double, std::string>> distances;
        distances.reserve(ids.size() - 1);
        for (const auto& target : ids) {
            if (source != target) {
                distances.emplace_back(distanceBetween(graph, source, target), target);
            }
        }
        std::sort(distances.begin(), distances.end());
        const auto nearestCount = std::min<std::size_t>(2, distances.size());
        for (std::size_t index = 0; index < nearestCount; ++index) {
            addRoadIfMissing(graph, source, distances[index].second);
        }
    }

    addDensityRoads(graph, config.roadDensity, rng);
    return graph;
}

graph::Graph generateCircularCity(const CityGenerationConfig& config, std::mt19937& rng) {
    graph::Graph graph{config.directed};
    const auto radius = std::max(config.averageRoadLength * 2.0, config.averageRoadLength * static_cast<double>(config.intersections) / (2.0 * kPi));

    for (std::size_t index = 0; index < config.intersections; ++index) {
        const auto angle = 2.0 * kPi * static_cast<double>(index) / static_cast<double>(config.intersections);
        graph.addNode(
            nodeId(index),
            radius * std::cos(angle),
            radius * std::sin(angle),
            {{"layout", "circular"}, {"angle", angle}});
    }

    for (std::size_t index = 0; index < config.intersections; ++index) {
        addRoadIfMissing(graph, nodeId(index), nodeId((index + 1) % config.intersections));
    }

    addDensityRoads(graph, config.roadDensity, rng);
    return graph;
}

graph::Graph generateRadialCity(const CityGenerationConfig& config, std::mt19937& rng) {
    graph::Graph graph{config.directed};
    graph.addNode(nodeId(0), 0.0, 0.0, {{"layout", "radial"}, {"ring", 0}});

    const auto spokes = std::max<std::size_t>(4, static_cast<std::size_t>(std::round(std::sqrt(static_cast<double>(config.intersections)))));
    for (std::size_t index = 1; index < config.intersections; ++index) {
        const auto spoke = (index - 1) % spokes;
        const auto ring = ((index - 1) / spokes) + 1;
        const auto angle = 2.0 * kPi * static_cast<double>(spoke) / static_cast<double>(spokes);
        const auto radius = static_cast<double>(ring) * config.averageRoadLength;
        graph.addNode(
            nodeId(index),
            radius * std::cos(angle),
            radius * std::sin(angle),
            {{"layout", "radial"}, {"ring", ring}, {"spoke", spoke}});
    }

    for (std::size_t index = 1; index < config.intersections; ++index) {
        const auto previousOnSpoke = index > spokes ? index - spokes : 0;
        addRoadIfMissing(graph, nodeId(previousOnSpoke), nodeId(index));

        const auto ringStart = ((index - 1) / spokes) * spokes + 1;
        const auto ringEnd = std::min(ringStart + spokes, config.intersections);
        const auto next = (index + 1 < ringEnd) ? index + 1 : ringStart;
        if (next != index && next < config.intersections) {
            addRoadIfMissing(graph, nodeId(index), nodeId(next));
        }
    }

    addDensityRoads(graph, config.roadDensity, rng);
    return graph;
}

}  // namespace

nlohmann::json CityGenerationConfig::toJson() const {
    return {
        {"layout", toString(layout)},
        {"intersections", intersections},
        {"roadDensity", roadDensity},
        {"averageRoadLength", averageRoadLength},
        {"directed", directed},
        {"seed", seed},
    };
}

CityGenerationConfig CityGenerationConfig::fromJson(const nlohmann::json& json) {
    CityGenerationConfig config;
    if (json.contains("layout")) {
        config.layout = cityLayoutFromString(json.at("layout").get<std::string>());
    }
    config.intersections = json.value("intersections", config.intersections);
    config.roadDensity = json.value("roadDensity", config.roadDensity);
    config.averageRoadLength = json.value("averageRoadLength", config.averageRoadLength);
    config.directed = json.value("directed", config.directed);
    config.seed = json.value("seed", config.seed);
    validateConfig(config);
    return config;
}

std::string toString(CityLayout layout) {
    switch (layout) {
        case CityLayout::Grid:
            return "grid";
        case CityLayout::Random:
            return "random";
        case CityLayout::Circular:
            return "circular";
        case CityLayout::Radial:
            return "radial";
    }
    return "grid";
}

CityLayout cityLayoutFromString(const std::string& value) {
    if (value == "grid" || value == "Grid") {
        return CityLayout::Grid;
    }
    if (value == "random" || value == "Random") {
        return CityLayout::Random;
    }
    if (value == "circular" || value == "Circular") {
        return CityLayout::Circular;
    }
    if (value == "radial" || value == "Radial") {
        return CityLayout::Radial;
    }
    throw std::invalid_argument("unknown city layout: " + value);
}

graph::Graph CityGenerator::generate(const CityGenerationConfig& config) {
    validateConfig(config);
    std::mt19937 rng(config.seed);

    switch (config.layout) {
        case CityLayout::Grid:
            return generateGrid(config, rng);
        case CityLayout::Random:
            return generateRandomCity(config, rng);
        case CityLayout::Circular:
            return generateCircularCity(config, rng);
        case CityLayout::Radial:
            return generateRadialCity(config, rng);
    }

    return generateGrid(config, rng);
}

void CityGenerator::exportCity(const graph::Graph& graph, const std::filesystem::path& path) {
    graph.saveToFile(path);
}

}  // namespace smarttraffic::city
