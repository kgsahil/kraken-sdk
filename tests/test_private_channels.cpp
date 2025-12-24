/// @file test_private_channels.cpp
/// @brief Unit tests for private channel data structures

#include <gtest/gtest.h>
#include <kraken/core/types.hpp>
#include <cmath>

using namespace kraken;

class OrderTest : public ::testing::Test {
protected:
    Order make_order(const std::string& order_id, const std::string& symbol,
                     Side side, double price, double quantity, double filled = 0.0) {
        Order order;
        order.order_id = order_id;
        order.symbol = symbol;
        order.side = side;
        order.type = OrderType::Limit;
        order.status = OrderStatus::Open;
        order.price = price;
        order.quantity = quantity;
        order.filled = filled;
        order.remaining = quantity - filled;
        order.timestamp = "2024-01-01T00:00:00Z";
        return order;
    }
};

TEST_F(OrderTest, FillPercentage) {
    Order order = make_order("123", "BTC/USD", Side::Buy, 50000.0, 1.0, 0.5);
    EXPECT_DOUBLE_EQ(order.fill_percentage(), 50.0);
    
    order.filled = 1.0;
    EXPECT_DOUBLE_EQ(order.fill_percentage(), 100.0);
    
    order.filled = 0.0;
    EXPECT_DOUBLE_EQ(order.fill_percentage(), 0.0);
}

TEST_F(OrderTest, IsFilled) {
    Order order = make_order("123", "BTC/USD", Side::Buy, 50000.0, 1.0, 0.5);
    EXPECT_FALSE(order.is_filled());
    
    order.filled = 1.0;
    EXPECT_TRUE(order.is_filled());
    
    order.filled = 1.1;  // Over-filled
    EXPECT_TRUE(order.is_filled());
}

TEST_F(OrderTest, ToJson) {
    Order order = make_order("order-123", "BTC/USD", Side::Buy, 50000.0, 1.0, 0.5);
    std::string json = order.to_json();
    
    EXPECT_NE(json.find("order-123"), std::string::npos);
    EXPECT_NE(json.find("BTC/USD"), std::string::npos);
    EXPECT_NE(json.find("buy"), std::string::npos);
    EXPECT_NE(json.find("50000"), std::string::npos);
    EXPECT_NE(json.find("50.00"), std::string::npos);  // fill_percent
}

class OwnTradeTest : public ::testing::Test {
protected:
    OwnTrade make_trade(const std::string& trade_id, const std::string& order_id,
                        const std::string& symbol, Side side, double price, 
                        double quantity, double fee = 0.0) {
        OwnTrade trade;
        trade.trade_id = trade_id;
        trade.order_id = order_id;
        trade.symbol = symbol;
        trade.side = side;
        trade.price = price;
        trade.quantity = quantity;
        trade.fee = fee;
        trade.fee_currency = "USD";
        trade.timestamp = "2024-01-01T00:00:00Z";
        return trade;
    }
};

TEST_F(OwnTradeTest, Value) {
    OwnTrade trade = make_trade("t1", "o1", "BTC/USD", Side::Buy, 50000.0, 1.0);
    EXPECT_DOUBLE_EQ(trade.value(), 50000.0);
    
    trade.quantity = 2.0;
    EXPECT_DOUBLE_EQ(trade.value(), 100000.0);
}

TEST_F(OwnTradeTest, NetValue) {
    OwnTrade trade = make_trade("t1", "o1", "BTC/USD", Side::Buy, 50000.0, 1.0, 10.0);
    EXPECT_DOUBLE_EQ(trade.net_value(), 49990.0);
    
    trade.fee = 0.0;
    EXPECT_DOUBLE_EQ(trade.net_value(), 50000.0);
}

TEST_F(OwnTradeTest, ToJson) {
    OwnTrade trade = make_trade("trade-123", "order-456", "BTC/USD", Side::Buy, 
                                50000.0, 1.0, 10.0);
    std::string json = trade.to_json();
    
    EXPECT_NE(json.find("trade-123"), std::string::npos);
    EXPECT_NE(json.find("order-456"), std::string::npos);
    EXPECT_NE(json.find("BTC/USD"), std::string::npos);
    EXPECT_NE(json.find("50000"), std::string::npos);
    EXPECT_NE(json.find("10"), std::string::npos);  // fee
}

class BalanceTest : public ::testing::Test {
protected:
    Balance make_balance(const std::string& currency, double available, double reserved) {
        Balance balance;
        balance.currency = currency;
        balance.available = available;
        balance.reserved = reserved;
        balance.total = available + reserved;
        return balance;
    }
};

TEST_F(BalanceTest, Total) {
    Balance balance = make_balance("BTC", 1.0, 0.5);
    EXPECT_DOUBLE_EQ(balance.total, 1.5);
    
    balance.available = 2.0;
    balance.reserved = 1.0;
    balance.total = balance.available + balance.reserved;
    EXPECT_DOUBLE_EQ(balance.total, 3.0);
}

TEST_F(BalanceTest, ToJson) {
    Balance balance = make_balance("BTC", 1.5, 0.5);
    std::string json = balance.to_json();
    
    EXPECT_NE(json.find("BTC"), std::string::npos);
    EXPECT_NE(json.find("1.5"), std::string::npos);
    EXPECT_NE(json.find("0.5"), std::string::npos);
    EXPECT_NE(json.find("2"), std::string::npos);  // total
}

