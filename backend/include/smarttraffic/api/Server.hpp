#pragma once

#include "smarttraffic/simulation/SimulationEngine.hpp"

#include <crow.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <unordered_set>

namespace smarttraffic::api {

class SmartTrafficServer final {
public:
    SmartTrafficServer();
    ~SmartTrafficServer();

    SmartTrafficServer(const SmartTrafficServer&) = delete;
    SmartTrafficServer& operator=(const SmartTrafficServer&) = delete;

    void run(std::uint16_t port);
    [[nodiscard]] simulation::SimulationEngine& engine() noexcept;

private:
    void configureRoutes();
    void startBroadcastLoop();
    void stopBroadcastLoop();

    [[nodiscard]] static crow::response jsonResponse(const nlohmann::json& body, int status = 200);
    [[nodiscard]] static crow::response errorResponse(const std::string& message, int status);
    [[nodiscard]] static nlohmann::json parseBody(const crow::request& request);
    [[nodiscard]] nlohmann::json firstAvailableRoad() const;

    simulation::SimulationEngine engine_;
    crow::SimpleApp app_;
    std::mutex connectionsMutex_;
    std::unordered_set<crow::websocket::connection*> connections_;
    std::atomic_bool broadcasting_{false};
    std::thread broadcastThread_;
};

}  // namespace smarttraffic::api
