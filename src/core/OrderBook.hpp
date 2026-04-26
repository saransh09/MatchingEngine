#pragma once
#include "Order.hpp"
#include "Trade.hpp"
#include "Types.hpp"
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <optional>

using Price = uint64_t;

class OrderBook {
public:
  OrderBook() = default;
  void add_order(Order &&order);
  std::optional<Order> remove_from_order(Side side, Price price);
  std::optional<std::reference_wrapper<const Order>> best_bid() const;
  std::optional<std::reference_wrapper<const Order>> best_ask() const;
  // std::optional<std::reference_wrapper<std::deque<Order>>>
  // get_orders_at_price(Price price) const;
  bool is_empty(Side side) const;
  std::vector<Trade> processOrder(Order &&order);

private:
  std::map<Price, std::deque<Order>> bids;
  std::map<Price, std::deque<Order>> asks;
  uint64_t next_trade_id = 0;
};
