cmake_minimum_required(VERSION 3.16)
project(KrakenSDK VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(KRAKEN_BUILD_EXAMPLES "Build example programs" ON)
option(KRAKEN_BUILD_TESTS "Build tests" ON)
option(KRAKEN_BUILD_TOOLS "Build tools (benchmark)" ON)

# Find system dependencies
# Note: Boost.Beast works with Boost 1.70+, but we prefer 1.81+ for best features
# Set policies for Boost finding - must be set before find_package
cmake_policy(SET CMP0144 NEW)
cmake_policy(SET CMP0167 OLD)  # Use FindBoost module (not removed yet)

# Force FindBoost instead of BoostConfig
# Unset Boost_DIR to prevent CMake from using BoostConfig.cmake
if(DEFINED Boost_DIR)
    unset(Boost_DIR CACHE)
endif()

# Try to find Boost using FindBoost module
# If BOOST_ROOT is set, use it; otherwise let CMake search
if(DEFINED ENV{BOOST_ROOT})
    set(BOOST_ROOT $ENV{BOOST_ROOT})
    set(Boost_NO_BOOST_CMAKE ON)  # Disable BoostConfig.cmake
endif()

# Set Boost options before find_package
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.70 REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Fetch external dependencies
include(FetchContent)

# rigtorp/SPSCQueue - Lock-free single producer single consumer queue
FetchContent_Declare(
    SPSCQueue
    GIT_REPOSITORY https://github.com/rigtorp/SPSCQueue.git
    GIT_TAG v1.1
)
FetchContent_MakeAvailable(SPSCQueue)

# RapidJSON - Fast JSON parsing
FetchContent_Declare(
    rapidjson
    GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
    GIT_TAG v1.1.0
)
FetchContent_GetProperties(rapidjson)
if(NOT rapidjson_POPULATED)
    FetchContent_Populate(rapidjson)
endif()

# spdlog - Fast C++ logging library
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)
FetchContent_MakeAvailable(spdlog)

# Main library
add_library(kraken STATIC
    src/client.cpp
    src/client_impl.cpp
    src/connection.cpp
    src/parser.cpp
    src/book_engine.cpp
    src/strategy_engine.cpp
    src/subscription.cpp
    src/config.cpp
    src/config_env.cpp
    src/telemetry.cpp
    src/queue.cpp
    src/logger.cpp
    src/auth.cpp
    src/rate_limiter.cpp
)

target_include_directories(kraken
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/internal
        ${rapidjson_SOURCE_DIR}/include
        ${SPSCQueue_SOURCE_DIR}/include
    )

target_link_libraries(kraken
    PUBLIC
        Threads::Threads
    PRIVATE
        Boost::system
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
)


target_compile_definitions(kraken PRIVATE
    BOOST_ASIO_NO_DEPRECATED
)

# Examples
if(KRAKEN_BUILD_EXAMPLES)
    # Common utilities for examples
    add_library(examples_common STATIC
        examples/common.cpp
        examples/config.cpp
    )
    target_include_directories(examples_common PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_include_directories(examples_common PRIVATE
        ${spdlog_SOURCE_DIR}/include
    )
    target_link_libraries(examples_common PRIVATE kraken)
    
    # Helper function to add example with spdlog include
    function(add_example name)
        add_executable(${name} examples/${name}.cpp)
        target_include_directories(${name} PRIVATE
            ${spdlog_SOURCE_DIR}/include
        )
        target_link_libraries(${name} PRIVATE kraken examples_common)
    endfunction()

    add_example(quickstart)
    add_example(strategies)
    add_example(dashboard)
    add_example(orderbook)
    add_example(telemetry)
    add_example(data_collector)
    add_example(trading_bot)
    add_example(web_backend)
endif()

# Tools
if(KRAKEN_BUILD_TOOLS)
    # Integration benchmark (end-to-end with live API)
    add_executable(benchmark_integration benchmarks/bench_backpressure.cpp)
    target_include_directories(benchmark_integration PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${spdlog_SOURCE_DIR}/include
    )
    target_link_libraries(benchmark_integration PRIVATE kraken)
    
    # Google Benchmark for microbenchmarks
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.8.3
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googlebenchmark)
    
    # Microbenchmark executables
    function(add_microbenchmark name)
        add_executable(${name} benchmarks/${name}.cpp)
        target_link_libraries(${name} PRIVATE kraken benchmark::benchmark benchmark::benchmark_main)
        target_include_directories(${name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${SPSCQueue_SOURCE_DIR}/include
            ${spdlog_SOURCE_DIR}/include
        )
    endfunction()
    
    add_microbenchmark(bench_parser)
    add_microbenchmark(bench_queue)
    add_microbenchmark(bench_orderbook)
    add_microbenchmark(bench_checksum)
    add_microbenchmark(bench_backpressure)
endif()

# Tests
if(KRAKEN_BUILD_TESTS)
    enable_testing()
    
    # Fetch Google Test
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
    
    # Test executables - all need access to src/ for internal headers
    function(add_kraken_test name)
        add_executable(${name} tests/${name}.cpp)
        target_include_directories(${name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/src/internal
            ${SPSCQueue_SOURCE_DIR}/include
            ${spdlog_SOURCE_DIR}/include
        )
        target_link_libraries(${name} PRIVATE kraken GTest::gtest_main)
        add_test(NAME ${name} COMMAND ${name})
    endfunction()
    
    add_kraken_test(test_strategies)
    add_kraken_test(test_book_checksum)
    add_kraken_test(test_connection)
    add_kraken_test(test_parser)
    add_kraken_test(test_client)
    add_kraken_test(test_config)
    add_kraken_test(test_subscription)
    add_kraken_test(test_error_handling)
    add_kraken_test(test_metrics)
    add_kraken_test(test_integration)
    add_kraken_test(test_message_flow)
    add_kraken_test(test_thread_safety)
    add_kraken_test(test_edge_cases)
    add_kraken_test(test_exception_safety)
    add_kraken_test(test_rate_limiter)
    add_kraken_test(test_backoff)
    add_kraken_test(test_gap_detector)
    add_kraken_test(test_telemetry)
    add_kraken_test(test_auth)
    add_kraken_test(test_logger)
    add_kraken_test(test_queue)
    add_kraken_test(test_config_env)
    add_kraken_test(test_connection_config)
    add_kraken_test(test_stress_failure)
endif()

# Installation
# Note: We don't export spdlog since it's a header-only dependency
# Users will need to include spdlog separately or we can make it optional
install(TARGETS kraken
    EXPORT KrakenSDKTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/kraken
    DESTINATION include
)

# Note: We skip the export for now due to spdlog dependency
# Users can link against kraken directly
# install(EXPORT KrakenSDKTargets
#     FILE KrakenSDKTargets.cmake
#     NAMESPACE kraken::
#     DESTINATION lib/cmake/KrakenSDK
# )

# Package config
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/KrakenSDKConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_file(cmake/KrakenSDKConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/KrakenSDKConfig.cmake"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/KrakenSDKConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/KrakenSDKConfigVersion.cmake"
    DESTINATION lib/cmake/KrakenSDK
)

