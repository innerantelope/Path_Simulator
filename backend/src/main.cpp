#include "smarttraffic/api/Server.hpp"

#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <string>

int main() {
    std::uint16_t port = 18080;
    if (const char* portValue = std::getenv("PORT")) {
        port = static_cast<std::uint16_t>(std::stoi(portValue));
    }

    std::cout << "SmartTraffic AI backend listening on port " << port << '\n';
    smarttraffic::api::SmartTrafficServer server;
    server.run(port);
    return 0;
}
