#include "smarttraffic/pathfinding/Pathfinder.hpp"

#include <gtest/gtest.h>

namespace smarttraffic::pathfinding {
namespace {

graph::Graph weightedGraph() {
    graph::Graph graph{true};
    graph.addNode("A", 0.0, 0.0);
    graph.addNode("B", 1.0, 0.0);
    graph.addNode("C", 0.0, 2.0);
    graph.addNode("D", 2.0, 0.0);
    graph.addEdge("A", "B", 1.0);
    graph.addEdge("B", "D", 1.0);
    graph.addEdge("A", "C", 4.0);
    graph.addEdge("C", "D", 1.0);
    return graph;
}

}  // namespace

TEST(PathfinderTest, AStarReturnsShortestPathCostAndVisitedNodes) {
    const auto graph = weightedGraph();

    const auto result = Pathfinder::aStar(graph, "A", "D");

    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.algorithm, "A*");
    EXPECT_EQ(result.path, (std::vector<std::string>{"A", "B", "D"}));
    EXPECT_DOUBLE_EQ(result.travelCost, 2.0);
    EXPECT_FALSE(result.visitedNodes.empty());
    EXPECT_GE(result.executionTimeMs, 0.0);
}

TEST(PathfinderTest, DijkstraMatchesAStarOnNonNegativeWeights) {
    const auto graph = weightedGraph();

    const auto astar = Pathfinder::aStar(graph, "A", "D");
    const auto dijkstra = Pathfinder::dijkstra(graph, "A", "D");

    ASSERT_TRUE(astar.success);
    ASSERT_TRUE(dijkstra.success);
    EXPECT_DOUBLE_EQ(dijkstra.travelCost, astar.travelCost);
    EXPECT_EQ(dijkstra.path, astar.path);
}

TEST(PathfinderTest, BFSReturnsFewestHopPathWithWeightedCost) {
    const auto graph = weightedGraph();

    const auto result = Pathfinder::breadthFirstSearch(graph, "A", "D");

    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.algorithm, "BFS");
    EXPECT_EQ(result.path.size(), 3U);
    EXPECT_DOUBLE_EQ(result.travelCost, 2.0);
}

TEST(PathfinderTest, SearchImmediatelyUsesUpdatedEdgeWeights) {
    auto graph = weightedGraph();
    ASSERT_TRUE(graph.updateEdgeWeight("A", "B", 100'000'000.0));

    const auto result = Pathfinder::aStar(graph, "A", "D");

    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.path, (std::vector<std::string>{"A", "C", "D"}));
    EXPECT_DOUBLE_EQ(result.travelCost, 5.0);
}

TEST(PathfinderTest, ThrowsForUnknownEndpointsAndAlgorithms) {
    const auto graph = weightedGraph();

    EXPECT_THROW(Pathfinder::aStar(graph, "missing", "D"), std::invalid_argument);
    EXPECT_THROW(Pathfinder::aStar(graph, "A", "missing"), std::invalid_argument);
    EXPECT_THROW(
        {
            const auto algorithm = algorithmFromString("bellman-ford");
            static_cast<void>(algorithm);
        },
        std::invalid_argument);
}

}  // namespace smarttraffic::pathfinding
