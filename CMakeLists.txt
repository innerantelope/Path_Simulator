cmake_minimum_required(VERSION 3.20)

project(
    SmartTrafficAI
    VERSION 0.1.0
    DESCRIPTION "Real-Time Smart Traffic Simulation powered by the A* Search Algorithm"
    LANGUAGES CXX
)

include(CTest)

add_subdirectory(backend)
