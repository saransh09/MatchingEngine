#pragma once

#include "Order.hpp"
#include "Trade.hpp"

#include <vector>
template <typename OrderBookType> class MatchingEngine {
public:
  MatchingEngine(OrderBookType &orderBook) : book_(orderBook) {}

  std::vector<Trade> processOrder(Order order) {
      return book_.processOrder(std::move(order));
  };
  OrderBookType &getOrderBook() const {
      return book_;
  };

private:
  OrderBookType &book_;
};
