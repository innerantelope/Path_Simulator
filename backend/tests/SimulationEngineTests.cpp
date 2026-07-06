#include "smarttraffic/simulation/SimulationEngine.hpp"

#include <gtest/gtest.h>

#include <algorithm>

namespace smarttraffic::simulation {

TEST(SimulationEngineTest, GeneratesCityAndSpawnsVehiclesWithRoutes) {
    SimulationEngine engine;
    engine.generateCity(city::CityGenerationConfig{city::CityLayout::Grid, 16, 0.12, 50.0, false, 11});

    const auto id = engine.spawnVehicle(SpawnVehicleRequest{VehicleType::Ambulance, "N0", "N15", 0.0});
    const auto vehicles = engine.vehiclesJson();

    ASSERT_EQ(vehicles.size(), 1U);
    EXPECT_EQ(vehicles.front().at("id"), id);
    EXPECT_EQ(vehicles.front().at("type"), "ambulance");
    EXPECT_TRUE(vehicles.front().at("emergency").get<bool>());
    EXPECT_GT(vehicles.front().at("currentPath").size(), 1U);
}

TEST(SimulationEngineTest, TicksVehiclesAndUpdatesLiveStats) {
    SimulationEngine engine;
    engine.generateCity(city::CityGenerationConfig{city::CityLayout::Grid, 9, 0.1, 40.0, false, 5});
    const auto id = engine.spawnVehicle(SpawnVehicleRequest{VehicleType::Car, "N0", "N8", 40.0});
    static_cast<void>(id);
    engine.start();

    for (int tick = 0; tick < 20; ++tick) {
        engine.tick(0.05);
    }

    const auto stats = engine.statsJson();
    EXPECT_TRUE(engine.running());
    EXPECT_GT(stats.at("tick").get<std::size_t>(), 0U);
    EXPECT_EQ(stats.at("activeVehicles").get<std::size_t>(), 1U);
    EXPECT_GT(stats.at("simulatedTimeSeconds").get<double>(), 0.0);
}

TEST(SimulationEngineTest, RoadClosureUpdatesRoutingAndCanReopen) {
    SimulationEngine engine;
    engine.generateCity(city::CityGenerationConfig{city::CityLayout::Grid, 9, 0.0, 40.0, false, 5});

    const auto before = engine.route("N0", "N2", pathfinding::Algorithm::AStar);
    ASSERT_TRUE(before.success);
    ASSERT_GE(before.path.size(), 2U);

    engine.closeRoad("N0", "N1");
    const auto afterClosure = engine.route("N0", "N2", pathfinding::Algorithm::AStar);
    ASSERT_TRUE(afterClosure.success);
    EXPECT_NE(afterClosure.path, before.path);

    engine.openRoad("N0", "N1");
    const auto afterOpen = engine.route("N0", "N2", pathfinding::Algorithm::AStar);
    ASSERT_TRUE(afterOpen.success);
    EXPECT_EQ(afterOpen.path, before.path);
}

TEST(SimulationEngineTest, AccidentsAffectAndRestoreRoadState) {
    SimulationEngine engine;
    engine.generateCity(city::CityGenerationConfig{city::CityLayout::Grid, 9, 0.0, 40.0, false, 5});

    const auto accident = engine.createAccident("N0", "N1", AccidentKind::PartialBlockage, 0.75);
    const auto roadsWithAccident = engine.roadStatesJson();
    const auto accidentRoad = std::find_if(roadsWithAccident.begin(), roadsWithAccident.end(), [](const auto& road) {
        return road.at("source") == "N0" && road.at("target") == "N1";
    });

    ASSERT_NE(accidentRoad, roadsWithAccident.end());
    EXPECT_TRUE(accidentRoad->at("accident").get<bool>());
    EXPECT_GT(accidentRoad->at("currentWeight").get<double>(), accidentRoad->at("baseWeight").get<double>());

    EXPECT_TRUE(engine.deleteAccident(accident.id));
    EXPECT_TRUE(engine.accidentsJson().empty());
}

TEST(SimulationEngineTest, SupportsPauseResumeResetAndSpeedControls) {
    SimulationEngine engine;
    engine.setSpeedMultiplier(5.0);
    EXPECT_DOUBLE_EQ(engine.speedMultiplier(), 5.0);

    engine.start();
    EXPECT_TRUE(engine.running());

    engine.pause();
    EXPECT_FALSE(engine.running());

    engine.resume();
    EXPECT_TRUE(engine.running());

    engine.reset();
    EXPECT_FALSE(engine.running());
    EXPECT_DOUBLE_EQ(engine.speedMultiplier(), 1.0);
    EXPECT_TRUE(engine.vehiclesJson().empty());
}

}  // namespace smarttraffic::simulation
