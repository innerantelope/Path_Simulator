#include "smarttraffic/graph/Graph.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace smarttraffic::graph {
namespace {

void validateIdentifier(std::string_view value, std::string_view fieldName) {
    if (value.empty()) {
        throw std::invalid_argument(std::string(fieldName) + " must not be empty");
    }
}

void validateCoordinate(double value, std::string_view fieldName) {
    if (!std::isfinite(value)) {
        throw std::invalid_argument(std::string(fieldName) + " must be finite");
    }
}

void validateWeight(double weight) {
    if (!std::isfinite(weight) || weight < 0.0) {
        throw std::invalid_argument("edge weight must be a finite non-negative value");
    }
}

void validateMetadata(const nlohmann::json& metadata) {
    if (!metadata.is_object()) {
        throw std::invalid_argument("metadata must be a JSON object");
    }
}

std::string asKey(std::string_view value) {
    return std::string(value);
}

bool eraseEdge(std::vector<Edge>& edges, std::string_view target) {
    const auto previousSize = edges.size();
    edges.erase(
        std::remove_if(
            edges.begin(),
            edges.end(),
            [target](const Edge& edge) { return edge.target() == target; }),
        edges.end());
    return edges.size() != previousSize;
}

std::vector<Edge>::const_iterator findEdgeIterator(const std::vector<Edge>& edges, std::string_view target) {
    return std::find_if(edges.begin(), edges.end(), [target](const Edge& edge) { return edge.target() == target; });
}

std::vector<Edge>::iterator findEdgeIterator(std::vector<Edge>& edges, std::string_view target) {
    return std::find_if(edges.begin(), edges.end(), [target](const Edge& edge) { return edge.target() == target; });
}

bool isCanonicalUndirectedEdge(const Edge& edge) {
    return edge.source() <= edge.target();
}

}  // namespace

Node::Node(std::string id, double x, double y, nlohmann::json metadata)
    : id_(std::move(id)), x_(x), y_(y), metadata_(std::move(metadata)) {
    validateIdentifier(id_, "node id");
    validateCoordinate(x_, "node x coordinate");
    validateCoordinate(y_, "node y coordinate");
    validateMetadata(metadata_);
}

const std::string& Node::id() const noexcept {
    return id_;
}

double Node::x() const noexcept {
    return x_;
}

double Node::y() const noexcept {
    return y_;
}

const nlohmann::json& Node::metadata() const noexcept {
    return metadata_;
}

void Node::setPosition(double x, double y) {
    validateCoordinate(x, "node x coordinate");
    validateCoordinate(y, "node y coordinate");
    x_ = x;
    y_ = y;
}

void Node::setMetadata(nlohmann::json metadata) {
    validateMetadata(metadata);
    metadata_ = std::move(metadata);
}

nlohmann::json Node::toJson() const {
    return {
        {"id", id_},
        {"x", x_},
        {"y", y_},
        {"metadata", metadata_},
    };
}

Node Node::fromJson(const nlohmann::json& json) {
    if (!json.is_object()) {
        throw std::invalid_argument("node JSON must be an object");
    }

    const auto metadata = json.contains("metadata") ? json.at("metadata") : nlohmann::json::object();
    return Node{
        json.at("id").get<std::string>(),
        json.at("x").get<double>(),
        json.at("y").get<double>(),
        metadata,
    };
}

Edge::Edge(std::string source, std::string target, double weight, nlohmann::json metadata)
    : source_(std::move(source)), target_(std::move(target)), weight_(weight), metadata_(std::move(metadata)) {
    validateIdentifier(source_, "edge source");
    validateIdentifier(target_, "edge target");
    validateWeight(weight_);
    validateMetadata(metadata_);
}

const std::string& Edge::source() const noexcept {
    return source_;
}

const std::string& Edge::target() const noexcept {
    return target_;
}

double Edge::weight() const noexcept {
    return weight_;
}

const nlohmann::json& Edge::metadata() const noexcept {
    return metadata_;
}

void Edge::setWeight(double weight) {
    validateWeight(weight);
    weight_ = weight;
}

void Edge::setMetadata(nlohmann::json metadata) {
    validateMetadata(metadata);
    metadata_ = std::move(metadata);
}

Edge Edge::reversed() const {
    return Edge{target_, source_, weight_, metadata_};
}

nlohmann::json Edge::toJson() const {
    return {
        {"source", source_},
        {"target", target_},
        {"weight", weight_},
        {"metadata", metadata_},
    };
}

Edge Edge::fromJson(const nlohmann::json& json) {
    if (!json.is_object()) {
        throw std::invalid_argument("edge JSON must be an object");
    }

    const auto metadata = json.contains("metadata") ? json.at("metadata") : nlohmann::json::object();
    return Edge{
        json.at("source").get<std::string>(),
        json.at("target").get<std::string>(),
        json.at("weight").get<double>(),
        metadata,
    };
}

Graph::Graph(bool directed) : directed_(directed) {}

bool Graph::directed() const noexcept {
    return directed_;
}

bool Graph::empty() const noexcept {
    return nodes_.empty();
}

std::size_t Graph::nodeCount() const noexcept {
    return nodes_.size();
}

std::size_t Graph::edgeCount() const {
    return edges().size();
}

bool Graph::hasNode(std::string_view id) const {
    return nodes_.contains(asKey(id));
}

bool Graph::hasEdge(std::string_view source, std::string_view target) const {
    const auto sourceIt = adjacency_.find(asKey(source));
    if (sourceIt == adjacency_.end()) {
        return false;
    }

    return findEdgeIterator(sourceIt->second, target) != sourceIt->second.end();
}

bool Graph::addNode(const Node& node) {
    const auto insertResult = nodes_.emplace(node.id(), node);
    if (insertResult.second) {
        adjacency_.try_emplace(node.id());
    }
    return insertResult.second;
}

bool Graph::addNode(std::string id, double x, double y, nlohmann::json metadata) {
    return addNode(Node{std::move(id), x, y, std::move(metadata)});
}

bool Graph::removeNode(std::string_view id) {
    const auto key = asKey(id);
    if (nodes_.erase(key) == 0) {
        return false;
    }

    adjacency_.erase(key);
    for (auto& adjacencyEntry : adjacency_) {
        eraseEdge(adjacencyEntry.second, id);
    }
    return true;
}

bool Graph::addEdge(const Edge& edge) {
    if (!hasNode(edge.source())) {
        throw std::invalid_argument("edge source node does not exist: " + edge.source());
    }
    if (!hasNode(edge.target())) {
        throw std::invalid_argument("edge target node does not exist: " + edge.target());
    }
    if (hasEdge(edge.source(), edge.target())) {
        return false;
    }

    adjacency_.at(edge.source()).push_back(edge);

    if (!directed_ && edge.source() != edge.target()) {
        adjacency_.at(edge.target()).push_back(edge.reversed());
    }

    return true;
}

bool Graph::addEdge(std::string source, std::string target, double weight, nlohmann::json metadata) {
    return addEdge(Edge{std::move(source), std::move(target), weight, std::move(metadata)});
}

bool Graph::removeEdge(std::string_view source, std::string_view target) {
    const auto sourceIt = adjacency_.find(asKey(source));
    if (sourceIt == adjacency_.end()) {
        return false;
    }

    bool removed = eraseEdge(sourceIt->second, target);

    if (!directed_ && source != target) {
        const auto targetIt = adjacency_.find(asKey(target));
        if (targetIt != adjacency_.end()) {
            removed = eraseEdge(targetIt->second, source) || removed;
        }
    }

    return removed;
}

bool Graph::updateEdgeWeight(std::string_view source, std::string_view target, double weight) {
    validateWeight(weight);

    const auto sourceIt = adjacency_.find(asKey(source));
    if (sourceIt == adjacency_.end()) {
        return false;
    }

    const auto edgeIt = findEdgeIterator(sourceIt->second, target);
    if (edgeIt == sourceIt->second.end()) {
        return false;
    }

    edgeIt->setWeight(weight);

    if (!directed_ && source != target) {
        const auto reverseIt = adjacency_.find(asKey(target));
        if (reverseIt != adjacency_.end()) {
            const auto reverseEdgeIt = findEdgeIterator(reverseIt->second, source);
            if (reverseEdgeIt != reverseIt->second.end()) {
                reverseEdgeIt->setWeight(weight);
            }
        }
    }

    return true;
}

const Node& Graph::node(std::string_view id) const {
    const auto it = nodes_.find(asKey(id));
    if (it == nodes_.end()) {
        throw std::out_of_range("node does not exist: " + asKey(id));
    }
    return it->second;
}

Node& Graph::node(std::string_view id) {
    const auto it = nodes_.find(asKey(id));
    if (it == nodes_.end()) {
        throw std::out_of_range("node does not exist: " + asKey(id));
    }
    return it->second;
}

const std::vector<Edge>& Graph::neighbors(std::string_view id) const {
    const auto it = adjacency_.find(asKey(id));
    if (it == adjacency_.end()) {
        throw std::out_of_range("node does not exist: " + asKey(id));
    }
    return it->second;
}

std::optional<Edge> Graph::findEdge(std::string_view source, std::string_view target) const {
    const auto sourceIt = adjacency_.find(asKey(source));
    if (sourceIt == adjacency_.end()) {
        return std::nullopt;
    }

    const auto edgeIt = findEdgeIterator(sourceIt->second, target);
    if (edgeIt == sourceIt->second.end()) {
        return std::nullopt;
    }

    return *edgeIt;
}

std::vector<Node> Graph::nodes() const {
    std::vector<Node> result;
    result.reserve(nodes_.size());
    for (const auto& nodeEntry : nodes_) {
        result.push_back(nodeEntry.second);
    }

    std::sort(result.begin(), result.end(), [](const Node& left, const Node& right) {
        return left.id() < right.id();
    });
    return result;
}

std::vector<Edge> Graph::edges() const {
    std::vector<Edge> result;
    for (const auto& adjacencyEntry : adjacency_) {
        for (const auto& edge : adjacencyEntry.second) {
            if (directed_ || isCanonicalUndirectedEdge(edge)) {
                result.push_back(edge);
            }
        }
    }

    std::sort(result.begin(), result.end(), [](const Edge& left, const Edge& right) {
        if (left.source() == right.source()) {
            return left.target() < right.target();
        }
        return left.source() < right.source();
    });
    return result;
}

const Graph::AdjacencyList& Graph::adjacencyList() const noexcept {
    return adjacency_;
}

void Graph::clear() {
    nodes_.clear();
    adjacency_.clear();
}

nlohmann::json Graph::toJson() const {
    nlohmann::json json;
    json["directed"] = directed_;
    json["nodes"] = nlohmann::json::array();
    json["edges"] = nlohmann::json::array();

    for (const auto& graphNode : nodes()) {
        json["nodes"].push_back(graphNode.toJson());
    }

    for (const auto& edge : edges()) {
        json["edges"].push_back(edge.toJson());
    }

    return json;
}

Graph Graph::fromJson(const nlohmann::json& json) {
    if (!json.is_object()) {
        throw std::invalid_argument("graph JSON must be an object");
    }
    if (!json.contains("nodes") || !json.at("nodes").is_array()) {
        throw std::invalid_argument("graph JSON must contain a nodes array");
    }
    if (!json.contains("edges") || !json.at("edges").is_array()) {
        throw std::invalid_argument("graph JSON must contain an edges array");
    }

    Graph graph{json.value("directed", true)};

    for (const auto& graphNode : json.at("nodes")) {
        if (!graph.addNode(Node::fromJson(graphNode))) {
            throw std::invalid_argument("duplicate node id in graph JSON");
        }
    }

    for (const auto& edge : json.at("edges")) {
        if (!graph.addEdge(Edge::fromJson(edge))) {
            throw std::invalid_argument("duplicate edge in graph JSON");
        }
    }

    return graph;
}

void Graph::saveToFile(const std::filesystem::path& path) const {
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("failed to open graph file for writing: " + path.string());
    }

    output << toJson().dump(4) << '\n';
}

Graph Graph::loadFromFile(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open graph file for reading: " + path.string());
    }

    nlohmann::json json;
    input >> json;
    return Graph::fromJson(json);
}

}  // namespace smarttraffic::graph
