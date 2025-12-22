/// @file test_logger.cpp
/// @brief Unit tests for structured logging

#include <gtest/gtest.h>
#include "kraken/logger.hpp"
#include <fstream>
#include <filesystem>
#include <cstdio>

using namespace kraken;

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Shutdown any existing logger
        Logger::shutdown();
    }
    
    void TearDown() override {
        // Clean up logger after each test
        Logger::shutdown();
    }
};

// Test default initialization
TEST_F(LoggerTest, DefaultInit) {
    Logger::init();
    
    EXPECT_TRUE(Logger::is_initialized());
    EXPECT_NE(Logger::get(), nullptr);
}

// Test custom initialization
TEST_F(LoggerTest, CustomInit) {
    Logger::init("debug", true, "");
    
    EXPECT_TRUE(Logger::is_initialized());
    EXPECT_NE(Logger::get(), nullptr);
}

// Test file logging
TEST_F(LoggerTest, FileLogging) {
    std::string test_file = "/tmp/kraken_test.log";
    
    // Remove file if it exists
    std::remove(test_file.c_str());
    
    Logger::init("info", false, test_file);
    
    EXPECT_TRUE(Logger::is_initialized());
    
    // Log something
    Logger::get()->info("Test log message");
    Logger::shutdown();  // Flush
    
    // Check file was created and contains message
    std::ifstream file(test_file);
    EXPECT_TRUE(file.good());
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    EXPECT_TRUE(content.find("Test log message") != std::string::npos);
    
    // Cleanup
    std::remove(test_file.c_str());
}

// Test console logging
TEST_F(LoggerTest, ConsoleLogging) {
    Logger::init("info", true, "");
    
    EXPECT_TRUE(Logger::is_initialized());
    
    // Should not crash when logging
    Logger::get()->info("Console test");
    Logger::get()->warn("Warning test");
    Logger::get()->error("Error test");
}

// Test log levels
TEST_F(LoggerTest, LogLevels) {
    Logger::init("warn", true, "");
    
    EXPECT_TRUE(Logger::is_initialized());
    
    // Should not crash
    Logger::get()->warn("Warning");
    Logger::get()->error("Error");
    Logger::get()->critical("Critical");
}

// Test set_level
TEST_F(LoggerTest, SetLevel) {
    Logger::init("info", true, "");
    
    Logger::set_level("debug");
    Logger::set_level("warn");
    Logger::set_level("error");
    
    // Should not crash
    EXPECT_TRUE(Logger::is_initialized());
}

// Test shutdown
TEST_F(LoggerTest, Shutdown) {
    Logger::init();
    EXPECT_TRUE(Logger::is_initialized());
    
    Logger::shutdown();
    EXPECT_FALSE(Logger::is_initialized());
}

// Test auto-initialization
TEST_F(LoggerTest, AutoInit) {
    Logger::shutdown();
    EXPECT_FALSE(Logger::is_initialized());
    
    // get() should auto-initialize
    auto logger = Logger::get();
    EXPECT_NE(logger, nullptr);
    EXPECT_TRUE(Logger::is_initialized());
}

// Test re-initialization
TEST_F(LoggerTest, ReInit) {
    Logger::init("info", true, "");
    EXPECT_TRUE(Logger::is_initialized());
    
    // Re-initialize should work (shuts down old, creates new)
    Logger::init("debug", false, "");
    EXPECT_TRUE(Logger::is_initialized());
}

// Test multiple sinks (console + file)
TEST_F(LoggerTest, MultipleSinks) {
    std::string test_file = "/tmp/kraken_multi_test.log";
    std::remove(test_file.c_str());
    
    Logger::init("info", true, test_file);
    
    Logger::get()->info("Multi-sink test");
    Logger::shutdown();
    
    // File should exist
    std::ifstream file(test_file);
    EXPECT_TRUE(file.good());
    
    std::remove(test_file.c_str());
}

