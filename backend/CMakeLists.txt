include(FetchContent)
include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

add_library(
    smarttraffic_core
    src/graph/Graph.cpp
)

target_include_directories(
    smarttraffic_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    smarttraffic_core
    PUBLIC
        nlohmann_json::nlohmann_json
)

target_compile_features(smarttraffic_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(smarttraffic_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(smarttraffic_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(SMARTTRAFFIC_BUILD_TESTS "Build SmartTraffic AI backend tests" ON)

if(BUILD_TESTING AND SMARTTRAFFIC_BUILD_TESTS)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(
        smarttraffic_graph_tests
        tests/GraphTests.cpp
    )

    target_link_libraries(
        smarttraffic_graph_tests
        PRIVATE
            smarttraffic_core
            GTest::gtest_main
    )

    if(MSVC)
        target_compile_options(smarttraffic_graph_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(smarttraffic_graph_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    include(GoogleTest)
    gtest_discover_tests(smarttraffic_graph_tests)
endif()
