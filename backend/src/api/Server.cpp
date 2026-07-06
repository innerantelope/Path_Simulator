#include "smarttraffic/api/Server.hpp"

#include <chrono>
#include <exception>
#include <string>

namespace smarttraffic::api {
namespace {

std::string bodyString(const nlohmann::json& body) {
    return body.dump();
}

void addCorsHeaders(crow::response& response) {
    response.set_header("Access-Control-Allow-Origin", "*");
    response.set_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
    response.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    response.set_header("Content-Type", "application/json");
}

std::string queryOrDefault(const crow::request& request, const char* key, const std::string& defaultValue) {
    const auto* value = request.url_params.get(key);
    return value == nullptr ? defaultValue : std::string(value);
}

}  // namespace

SmartTrafficServer::SmartTrafficServer() {
    configureRoutes();
    startBroadcastLoop();
}

SmartTrafficServer::~SmartTrafficServer() {
    stopBroadcastLoop();
}

void SmartTrafficServer::run(std::uint16_t port) {
    app_.port(port).multithreaded().run();
}

simulation::SimulationEngine& SmartTrafficServer::engine() noexcept {
    return engine_;
}

void SmartTrafficServer::configureRoutes() {
    CROW_ROUTE(app_, "/health").methods(crow::HTTPMethod::Get)([] {
        return jsonResponse({{"status", "ok"}, {"service", "SmartTraffic AI"}});
    });

    CROW_ROUTE(app_, "/city").methods(crow::HTTPMethod::Get)([this] {
        return jsonResponse(engine_.cityJson());
    });

    CROW_ROUTE(app_, "/city/random").methods(crow::HTTPMethod::Post)([this](const crow::request& request) {
        try {
            const auto body = parseBody(request);
            const auto config = city::CityGenerationConfig::fromJson(body);
            const auto vehicleCount = body.value("vehicles", 80U);

            engine_.generateCity(config);
            const auto ids = engine_.spawnVehicles(vehicleCount, simulation::VehicleType::Car);
            return jsonResponse({{"city", engine_.cityJson()}, {"vehicleIds", ids}, {"vehicles", engine_.vehiclesJson()}}, 201);
        } catch (const std::exception& error) {
            return errorResponse(error.what(), 400);
        }
    });

    CROW_ROUTE(app_, "/vehicles").methods(crow::HTTPMethod::Get)([this] {
        return jsonResponse(engine_.vehiclesJson());
    });

    CROW_ROUTE(app_, "/vehicles").methods(crow::HTTPMethod::Post)([this](const crow::request& request) {
        try {
            const auto body = parseBody(request);
            const auto count = body.value("count", 1U);
            const auto type = simulation::vehicleTypeFromString(body.value("type", std::string{"car"}));
            std::vector<std::string> ids;

            if (count <= 1) {
                ids.push_back(engine_.spawnVehicle(simulation::SpawnVehicleRequest{
                    type,
                    body.value("source", std::string{}),
                    body.value("destination", std::string{}),
                    body.value("speed", 0.0),
                }));
            } else {
                ids = engine_.spawnVehicles(count, type);
            }

            return jsonResponse({{"ids", ids}, {"vehicles", engine_.vehiclesJson()}}, 201);
        } catch (const std::exception& error) {
            return errorResponse(error.what(), 400);
        }
    });

    CROW_ROUTE(app_, "/vehicles/<string>").methods(crow::HTTPMethod::Delete)([this](const std::string& id) {
        if (!engine_.removeVehicle(id)) {
            return errorResponse("vehicle not found: " + id, 404);
        }
        return jsonResponse({{"deleted", id}});
    });

    CROW_ROUTE(app_, "/road/close").methods(crow::HTTPMethod::Post)([this](const crow::request& request) {
        try {
            const auto body = parseBody(request);
            engine_.closeRoad(body.at("source").get<std::string>(), body.at("target").get<std::string>());
            return jsonResponse({{"roads", engine_.roadStatesJson()}});
        } catch (const std::exception& error) {
            return errorResponse(error.what(), 400);
        }
    });

    CROW_ROUTE(app_, "/road/open").methods(crow::HTTPMethod::Post)([this](const crow::request& request) {
        try {
            const auto body = parseBody(request);
            engine_.openRoad(body.at("source").get<std::string>(), body.at("target").get<std::string>());
            return jsonResponse({{"roads", engine_.roadStatesJson()}});
        } catch (const std::exception& error) {
            return errorResponse(error.what(), 400);
        }
    });

    CROW_ROUTE(app_, "/accident").methods(crow::HTTPMethod::Post)([this](const crow::request& request) {
        try {
            const auto body = parseBody(request);
            const auto road = body.contains("source") && body.contains("target")
                ? body
                : firstAvailableRoad();
            const auto accident = engine_.createAccident(
                road.at("source").get<std::string>(),
                road.at("target").get<std::string>(),
                simulation::accidentKindFromString(body.value("kind", std::string{"partial_blockage"})),
                body.value("severity", 0.65));
            return jsonResponse({{"accident", accident.toJson()}, {"roads", engine_.roadStatesJson()}}, 201);
        } catch (const std::exception& error) {
            return errorResponse(error.what(), 400);
        }
    });

    CROW_ROUTE(app_, "/accident").methods(crow::HTTPMethod::Delete)([this](const crow::request& request) {
        try {
            const auto body = parseBody(request);
            if (!body.contains("id")) {
                engine_.clearAccidents();
                return jsonResponse({{"cleared", true}, {"accidents", engine_.accidentsJson()}});
            }

            const auto id = body.at("id").get<std::string>();
            if (!engine_.deleteAccident(id)) {
                return errorResponse("accident not found: " + id, 404);
            }
            return jsonResponse({{"deleted", id}, {"accidents", engine_.accidentsJson()}});
        } catch (const std::exception& error) {
            return errorResponse(error.what(), 400);
        }
    });

    CROW_ROUTE(app_, "/stats").methods(crow::HTTPMethod::Get)([this] {
        return jsonResponse(engine_.statsJson());
    });

    CROW_ROUTE(app_, "/simulation").methods(crow::HTTPMethod::Get)([this] {
        return jsonResponse(engine_.simulationJson());
    });

    CROW_ROUTE(app_, "/simulation/start").methods(crow::HTTPMethod::Post)([this] {
        engine_.start();
        return jsonResponse(engine_.simulationJson());
    });

    CROW_ROUTE(app_, "/simulation/pause").methods(crow::HTTPMethod::Post)([this] {
        engine_.pause();
        return jsonResponse(engine_.simulationJson());
    });

    CROW_ROUTE(app_, "/simulation/resume").methods(crow::HTTPMethod::Post)([this] {
        engine_.resume();
        return jsonResponse(engine_.simulationJson());
    });

    CROW_ROUTE(app_, "/simulation/reset").methods(crow::HTTPMethod::Post)([this] {
        engine_.reset();
        const auto ids = engine_.spawnVehicles(40, simulation::VehicleType::Car);
        static_cast<void>(ids);
        return jsonResponse(engine_.simulationJson());
    });

    CROW_ROUTE(app_, "/simulation/speed").methods(crow::HTTPMethod::Post)([this](const crow::request& request) {
        try {
            const auto body = parseBody(request);
            engine_.setSpeedMultiplier(body.value("speed", 1.0));
            return jsonResponse(engine_.simulationJson());
        } catch (const std::exception& error) {
            return errorResponse(error.what(), 400);
        }
    });

    CROW_ROUTE(app_, "/route").methods(crow::HTTPMethod::Get)([this](const crow::request& request) {
        try {
            const auto source = queryOrDefault(request, "source", "");
            const auto destination = queryOrDefault(request, "destination", "");
            const auto algorithm = pathfinding::algorithmFromString(queryOrDefault(request, "algorithm", "A*"));
            return jsonResponse(engine_.route(source, destination, algorithm).toJson());
        } catch (const std::exception& error) {
            return errorResponse(error.what(), 400);
        }
    });

    CROW_ROUTE(app_, "/algorithms/compare").methods(crow::HTTPMethod::Get)([this](const crow::request& request) {
        try {
            const auto source = queryOrDefault(request, "source", "");
            const auto destination = queryOrDefault(request, "destination", "");
            return jsonResponse({
                {"A*", engine_.route(source, destination, pathfinding::Algorithm::AStar).toJson()},
                {"Dijkstra", engine_.route(source, destination, pathfinding::Algorithm::Dijkstra).toJson()},
                {"BFS", engine_.route(source, destination, pathfinding::Algorithm::BFS).toJson()},
            });
        } catch (const std::exception& error) {
            return errorResponse(error.what(), 400);
        }
    });

    CROW_WEBSOCKET_ROUTE(app_, "/ws")
        .onopen([this](crow::websocket::connection& connection) {
            std::lock_guard lock(connectionsMutex_);
            connections_.insert(&connection);
            connection.send_text(bodyString(engine_.snapshotJson()));
        })
        .onclose([this](crow::websocket::connection& connection, const std::string&) {
            std::lock_guard lock(connectionsMutex_);
            connections_.erase(&connection);
        })
        .onmessage([](crow::websocket::connection&, const std::string&, bool) {});
}

void SmartTrafficServer::startBroadcastLoop() {
    if (broadcasting_.exchange(true)) {
        return;
    }

    broadcastThread_ = std::thread([this] {
        using namespace std::chrono_literals;
        while (broadcasting_) {
            engine_.tick(0.05);
            const auto payload = bodyString(engine_.snapshotJson());
            {
                std::lock_guard lock(connectionsMutex_);
                for (auto* connection : connections_) {
                    connection->send_text(payload);
                }
            }
            std::this_thread::sleep_for(50ms);
        }
    });
}

void SmartTrafficServer::stopBroadcastLoop() {
    broadcasting_ = false;
    if (broadcastThread_.joinable()) {
        broadcastThread_.join();
    }
}

crow::response SmartTrafficServer::jsonResponse(const nlohmann::json& body, int status) {
    crow::response response(status);
    addCorsHeaders(response);
    response.write(bodyString(body));
    return response;
}

crow::response SmartTrafficServer::errorResponse(const std::string& message, int status) {
    return jsonResponse({{"error", message}}, status);
}

nlohmann::json SmartTrafficServer::parseBody(const crow::request& request) {
    if (request.body.empty()) {
        return nlohmann::json::object();
    }
    return nlohmann::json::parse(request.body);
}

nlohmann::json SmartTrafficServer::firstAvailableRoad() const {
    const auto roads = engine_.roadStatesJson();
    for (const auto& road : roads) {
        if (!road.value("closed", false)) {
            return road;
        }
    }
    throw std::runtime_error("no available roads");
}

}  // namespace smarttraffic::api
