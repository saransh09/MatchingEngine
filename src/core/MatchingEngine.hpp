#pragma once

#include "Trade.hpp"
#include "OrderBook.hpp"
#include <vector>
class MatchingEngine {
public:
    MatchingEngine(OrderBook& orderBook): book_(orderBook) {}
    std::vector<Trade> processOrder(Order order);
    OrderBook& getOrderBook() const;
private:
    std::vector<Trade> trades_;
    OrderBook& book_;
};
