#pragma once

#include "smarttraffic/graph/Graph.hpp"

#include <filesystem>
#include <string>

namespace smarttraffic::city {

enum class CityLayout {
    Grid,
    Random,
    Circular,
    Radial,
};

struct CityGenerationConfig {
    CityLayout layout{CityLayout::Grid};
    std::size_t intersections{100};
    double roadDensity{0.18};
    double averageRoadLength{80.0};
    bool directed{false};
    unsigned int seed{42};

    [[nodiscard]] nlohmann::json toJson() const;
    [[nodiscard]] static CityGenerationConfig fromJson(const nlohmann::json& json);
};

[[nodiscard]] std::string toString(CityLayout layout);
[[nodiscard]] CityLayout cityLayoutFromString(const std::string& value);

class CityGenerator final {
public:
    [[nodiscard]] static graph::Graph generate(const CityGenerationConfig& config);
    static void exportCity(const graph::Graph& graph, const std::filesystem::path& path);

private:
    CityGenerator() = default;
};

}  // namespace smarttraffic::city
