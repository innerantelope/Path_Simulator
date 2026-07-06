#pragma once

#include <nlohmann/json.hpp>

#include <string>

namespace smarttraffic::graph {

class Edge final {
public:
    Edge() = default;
    Edge(std::string source, std::string target, double weight, nlohmann::json metadata = nlohmann::json::object());

    [[nodiscard]] const std::string& source() const noexcept;
    [[nodiscard]] const std::string& target() const noexcept;
    [[nodiscard]] double weight() const noexcept;
    [[nodiscard]] const nlohmann::json& metadata() const noexcept;

    void setWeight(double weight);
    void setMetadata(nlohmann::json metadata);

    [[nodiscard]] Edge reversed() const;
    [[nodiscard]] nlohmann::json toJson() const;
    [[nodiscard]] static Edge fromJson(const nlohmann::json& json);

private:
    std::string source_;
    std::string target_;
    double weight_{0.0};
    nlohmann::json metadata_{nlohmann::json::object()};
};

}  // namespace smarttraffic::graph
