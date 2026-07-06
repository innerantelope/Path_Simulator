#pragma once

#include <nlohmann/json.hpp>

#include <string>

namespace smarttraffic::simulation {

enum class TrafficLightState {
    Green,
    Yellow,
    Red,
};

[[nodiscard]] std::string toString(TrafficLightState state);

class TrafficLight final {
public:
    TrafficLight() = default;
    explicit TrafficLight(std::string nodeId);

    [[nodiscard]] const std::string& nodeId() const noexcept;
    [[nodiscard]] TrafficLightState state() const noexcept;
    [[nodiscard]] double timer() const noexcept;

    void update(double deltaSeconds);
    void forceGreen(double seconds);
    [[nodiscard]] nlohmann::json toJson() const;

private:
    std::string nodeId_;
    TrafficLightState state_{TrafficLightState::Green};
    double timer_{0.0};
    double priorityHoldSeconds_{0.0};
    double greenSeconds_{18.0};
    double yellowSeconds_{3.0};
    double redSeconds_{14.0};
};

}  // namespace smarttraffic::simulation
