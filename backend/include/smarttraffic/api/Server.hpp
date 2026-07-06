#pragma once

#include "smarttraffic/simulation/SimulationEngine.hpp"

#include <crow.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <unordered_set>

namespace smarttraffic::api {

struct CorsMiddleware {
    struct context {};

    void before_handle(crow::request& request, crow::response& response, context&) {
        if (crow::method_name(request.method) == "OPTIONS") {
            apply(request, response);
            response.code = 200;
            response.set_header("Content-Type", "text/plain");
            response.end();
        }
    }

    void after_handle(crow::request& request, crow::response& response, context&) {
        apply(request, response);
    }

private:
    static void apply(crow::request& request, crow::response& response) {
        const auto origin = request.get_header_value("Origin");
        response.set_header("Access-Control-Max-Age", "86400");
        response.set_header("Vary", "Origin");
        response.set_header("Access-Control-Allow-Origin", origin.empty() ? "*" : origin);
        response.set_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
        response.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, Accept");
    }
};

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
    crow::App<CorsMiddleware> app_;
    std::mutex connectionsMutex_;
    std::unordered_set<crow::websocket::connection*> connections_;
    std::atomic_bool broadcasting_{false};
    std::thread broadcastThread_;
};

}  // namespace smarttraffic::api
