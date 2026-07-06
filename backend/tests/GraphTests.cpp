#include "smarttraffic/graph/Graph.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <limits>
#include <string>

namespace smarttraffic::graph {
namespace {

Graph sampleDirectedGraph() {
    Graph graph{true};
    graph.addNode("A", 0.0, 0.0, {{"kind", "intersection"}});
    graph.addNode("B", 3.0, 4.0);
    graph.addNode("C", 6.0, 8.0);
    graph.addEdge("A", "B", 5.0, {{"lanes", 2}});
    graph.addEdge("B", "C", 7.5);
    return graph;
}

std::filesystem::path uniqueTempGraphPath() {
    const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() / ("smarttraffic_graph_" + std::to_string(timestamp) + ".json");
}

}  // namespace

TEST(NodeTest, ValidatesIdentityCoordinatesAndMetadata) {
    const Node node{"N1", 10.0, 20.0, {{"zone", "north"}}};

    EXPECT_EQ(node.id(), "N1");
    EXPECT_DOUBLE_EQ(node.x(), 10.0);
    EXPECT_DOUBLE_EQ(node.y(), 20.0);
    EXPECT_EQ(node.metadata().at("zone"), "north");

    EXPECT_THROW(Node("", 0.0, 0.0), std::invalid_argument);
    EXPECT_THROW(Node("bad", std::numeric_limits<double>::infinity(), 0.0), std::invalid_argument);
    EXPECT_THROW(Node("bad", 0.0, 0.0, nlohmann::json::array()), std::invalid_argument);
}

TEST(EdgeTest, ValidatesEndpointsWeightAndMetadata) {
    const Edge edge{"A", "B", 12.5, {{"roadType", "arterial"}}};

    EXPECT_EQ(edge.source(), "A");
    EXPECT_EQ(edge.target(), "B");
    EXPECT_DOUBLE_EQ(edge.weight(), 12.5);
    EXPECT_EQ(edge.reversed().source(), "B");
    EXPECT_EQ(edge.reversed().target(), "A");

    EXPECT_THROW(Edge("", "B", 1.0), std::invalid_argument);
    EXPECT_THROW(Edge("A", "", 1.0), std::invalid_argument);
    EXPECT_THROW(Edge("A", "B", -1.0), std::invalid_argument);
    EXPECT_THROW(Edge("A", "B", 1.0, nlohmann::json::array()), std::invalid_argument);
}

TEST(GraphTest, StoresDirectedEdgesInOneDirection) {
    const Graph graph = sampleDirectedGraph();

    EXPECT_TRUE(graph.directed());
    EXPECT_EQ(graph.nodeCount(), 3U);
    EXPECT_EQ(graph.edgeCount(), 2U);
    EXPECT_TRUE(graph.hasEdge("A", "B"));
    EXPECT_FALSE(graph.hasEdge("B", "A"));
    EXPECT_EQ(graph.neighbors("A").size(), 1U);
    EXPECT_EQ(graph.neighbors("C").size(), 0U);
}

TEST(GraphTest, MirrorsUndirectedEdgesInAdjacencyList) {
    Graph graph{false};
    graph.addNode("A", 0.0, 0.0);
    graph.addNode("B", 1.0, 1.0);

    EXPECT_TRUE(graph.addEdge("B", "A", 2.0));

    EXPECT_FALSE(graph.directed());
    EXPECT_TRUE(graph.hasEdge("A", "B"));
    EXPECT_TRUE(graph.hasEdge("B", "A"));
    EXPECT_EQ(graph.edgeCount(), 1U);
    EXPECT_DOUBLE_EQ(graph.findEdge("A", "B")->weight(), 2.0);
}

TEST(GraphTest, UpdatesAndRemovesEdgesDynamically) {
    Graph graph{false};
    graph.addNode("A", 0.0, 0.0);
    graph.addNode("B", 1.0, 1.0);
    graph.addEdge("A", "B", 2.0);

    EXPECT_TRUE(graph.updateEdgeWeight("A", "B", 9.25));
    EXPECT_DOUBLE_EQ(graph.findEdge("A", "B")->weight(), 9.25);
    EXPECT_DOUBLE_EQ(graph.findEdge("B", "A")->weight(), 9.25);

    EXPECT_TRUE(graph.removeEdge("B", "A"));
    EXPECT_FALSE(graph.hasEdge("A", "B"));
    EXPECT_FALSE(graph.hasEdge("B", "A"));
    EXPECT_FALSE(graph.updateEdgeWeight("A", "B", 3.0));
}

TEST(GraphTest, RemovesNodesAndIncidentEdges) {
    Graph graph = sampleDirectedGraph();

    EXPECT_TRUE(graph.removeNode("B"));

    EXPECT_EQ(graph.nodeCount(), 2U);
    EXPECT_FALSE(graph.hasNode("B"));
    EXPECT_FALSE(graph.hasEdge("A", "B"));
    EXPECT_FALSE(graph.hasEdge("B", "C"));
    EXPECT_THROW(
        {
            const auto& missingNeighbors = graph.neighbors("B");
            static_cast<void>(missingNeighbors);
        },
        std::out_of_range);
}

TEST(GraphTest, RejectsEdgesWithMissingNodesAndDuplicateJsonEdges) {
    Graph graph{true};
    graph.addNode("A", 0.0, 0.0);

    EXPECT_THROW(graph.addEdge("A", "B", 1.0), std::invalid_argument);
    EXPECT_THROW(graph.addEdge("B", "A", 1.0), std::invalid_argument);

    const nlohmann::json duplicateEdgeJson = {
        {"directed", true},
        {"nodes", {{{"id", "A"}, {"x", 0.0}, {"y", 0.0}}, {{"id", "B"}, {"x", 1.0}, {"y", 1.0}}}},
        {"edges", {{{"source", "A"}, {"target", "B"}, {"weight", 1.0}}, {{"source", "A"}, {"target", "B"}, {"weight", 2.0}}}},
    };

    EXPECT_THROW(Graph::fromJson(duplicateEdgeJson), std::invalid_argument);
}

TEST(GraphTest, SerializesAndDeserializesDeterministically) {
    const Graph graph = sampleDirectedGraph();
    const auto json = graph.toJson();
    const auto roundTripped = Graph::fromJson(json);

    EXPECT_EQ(roundTripped.toJson(), json);
    EXPECT_TRUE(roundTripped.directed());
    EXPECT_EQ(roundTripped.node("A").metadata().at("kind"), "intersection");
    EXPECT_EQ(roundTripped.findEdge("A", "B")->metadata().at("lanes"), 2);
}

TEST(GraphTest, SavesAndLoadsJsonFiles) {
    const Graph graph = sampleDirectedGraph();
    const auto path = uniqueTempGraphPath();

    graph.saveToFile(path);
    const auto loaded = Graph::loadFromFile(path);
    std::filesystem::remove(path);

    EXPECT_EQ(loaded.toJson(), graph.toJson());
}

}  // namespace smarttraffic::graph
