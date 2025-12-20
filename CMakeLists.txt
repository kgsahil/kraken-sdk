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
)

target_include_directories(kraken
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
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
)

target_compile_definitions(kraken PRIVATE
    BOOST_ASIO_NO_DEPRECATED
)

# Examples
if(KRAKEN_BUILD_EXAMPLES)
    add_executable(quickstart examples/quickstart.cpp)
    target_link_libraries(quickstart PRIVATE kraken)

    add_executable(strategies examples/strategies.cpp)
    target_link_libraries(strategies PRIVATE kraken)

    add_executable(dashboard examples/dashboard.cpp)
    target_link_libraries(dashboard PRIVATE kraken)

    add_executable(orderbook examples/orderbook.cpp)
    target_link_libraries(orderbook PRIVATE kraken)
endif()

# Tools
if(KRAKEN_BUILD_TOOLS)
    add_executable(benchmark tools/benchmark.cpp)
    target_link_libraries(benchmark PRIVATE kraken)
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
    
    add_executable(test_strategies tests/test_strategies.cpp)
    target_link_libraries(test_strategies PRIVATE kraken GTest::gtest_main)
    add_test(NAME test_strategies COMMAND test_strategies)
    
    add_executable(test_book_checksum tests/test_book_checksum.cpp)
    target_link_libraries(test_book_checksum PRIVATE kraken GTest::gtest_main)
    add_test(NAME test_book_checksum COMMAND test_book_checksum)
endif()

# Installation
install(TARGETS kraken
    EXPORT KrakenSDKTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/kraken
    DESTINATION include
)

install(EXPORT KrakenSDKTargets
    FILE KrakenSDKTargets.cmake
    NAMESPACE kraken::
    DESTINATION lib/cmake/KrakenSDK
)

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

