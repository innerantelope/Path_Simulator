#pragma once

#include <nlohmann/json.hpp>

#include <string>

namespace smarttraffic::graph {

class Node final {
public:
    Node() = default;
    Node(std::string id, double x, double y, nlohmann::json metadata = nlohmann::json::object());

    [[nodiscard]] const std::string& id() const noexcept;
    [[nodiscard]] double x() const noexcept;
    [[nodiscard]] double y() const noexcept;
    [[nodiscard]] const nlohmann::json& metadata() const noexcept;

    void setPosition(double x, double y);
    void setMetadata(nlohmann::json metadata);

    [[nodiscard]] nlohmann::json toJson() const;
    [[nodiscard]] static Node fromJson(const nlohmann::json& json);

private:
    std::string id_;
    double x_{0.0};
    double y_{0.0};
    nlohmann::json metadata_{nlohmann::json::object()};
};

}  // namespace smarttraffic::graph
