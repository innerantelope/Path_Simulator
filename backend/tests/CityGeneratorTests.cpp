#include "smarttraffic/city/CityGenerator.hpp"

#include <gtest/gtest.h>

namespace smarttraffic::city {

TEST(CityGeneratorTest, GeneratesEverySupportedLayout) {
    for (const auto layout : {CityLayout::Grid, CityLayout::Random, CityLayout::Circular, CityLayout::Radial}) {
        const auto graph = CityGenerator::generate(CityGenerationConfig{layout, 36, 0.16, 75.0, false, 7});

        EXPECT_EQ(graph.nodeCount(), 36U);
        EXPECT_GT(graph.edgeCount(), 0U);
        EXPECT_FALSE(graph.directed());
    }
}

TEST(CityGeneratorTest, RandomLayoutIsDeterministicForTheSameSeed) {
    const CityGenerationConfig config{CityLayout::Random, 28, 0.22, 90.0, false, 99};

    const auto first = CityGenerator::generate(config).toJson();
    const auto second = CityGenerator::generate(config).toJson();

    EXPECT_EQ(first, second);
}

TEST(CityGeneratorTest, HonorsDirectedGraphConfiguration) {
    const auto graph = CityGenerator::generate(CityGenerationConfig{CityLayout::Circular, 12, 0.2, 50.0, true, 3});

    EXPECT_TRUE(graph.directed());
    EXPECT_TRUE(graph.hasEdge("N0", "N1"));
    EXPECT_FALSE(graph.hasEdge("N1", "N0"));
}

TEST(CityGeneratorTest, RejectsInvalidGenerationConfig) {
    EXPECT_THROW(
        CityGenerator::generate(CityGenerationConfig{CityLayout::Grid, 1, 0.2, 50.0, false, 1}),
        std::invalid_argument);
    EXPECT_THROW(
        CityGenerator::generate(CityGenerationConfig{CityLayout::Grid, 10, 1.2, 50.0, false, 1}),
        std::invalid_argument);
    EXPECT_THROW(
        CityGenerator::generate(CityGenerationConfig{CityLayout::Grid, 10, 0.2, -1.0, false, 1}),
        std::invalid_argument);
}

}  // namespace smarttraffic::city
