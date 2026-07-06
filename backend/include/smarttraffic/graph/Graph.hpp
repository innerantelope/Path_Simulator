#pragma once

#include "smarttraffic/graph/Edge.hpp"
#include "smarttraffic/graph/Node.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace smarttraffic::graph {

class Graph final {
public:
    using AdjacencyList = std::unordered_map<std::string, std::vector<Edge>>;

    explicit Graph(bool directed = true);

    [[nodiscard]] bool directed() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t nodeCount() const noexcept;
    [[nodiscard]] std::size_t edgeCount() const;

    [[nodiscard]] bool hasNode(std::string_view id) const;
    [[nodiscard]] bool hasEdge(std::string_view source, std::string_view target) const;

    bool addNode(const Node& node);
    bool addNode(std::string id, double x, double y, nlohmann::json metadata = nlohmann::json::object());
    bool removeNode(std::string_view id);

    bool addEdge(const Edge& edge);
    bool addEdge(std::string source, std::string target, double weight, nlohmann::json metadata = nlohmann::json::object());
    bool removeEdge(std::string_view source, std::string_view target);
    bool updateEdgeWeight(std::string_view source, std::string_view target, double weight);

    [[nodiscard]] const Node& node(std::string_view id) const;
    [[nodiscard]] Node& node(std::string_view id);
    [[nodiscard]] const std::vector<Edge>& neighbors(std::string_view id) const;
    [[nodiscard]] std::optional<Edge> findEdge(std::string_view source, std::string_view target) const;

    [[nodiscard]] std::vector<Node> nodes() const;
    [[nodiscard]] std::vector<Edge> edges() const;
    [[nodiscard]] const AdjacencyList& adjacencyList() const noexcept;

    void clear();

    [[nodiscard]] nlohmann::json toJson() const;
    [[nodiscard]] static Graph fromJson(const nlohmann::json& json);

    void saveToFile(const std::filesystem::path& path) const;
    [[nodiscard]] static Graph loadFromFile(const std::filesystem::path& path);

private:
    bool directed_{true};
    std::unordered_map<std::string, Node> nodes_;
    AdjacencyList adjacency_;
};

}  // namespace smarttraffic::graph
