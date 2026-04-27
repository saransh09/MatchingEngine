#pragma once

#include "Order.hpp"
#include "Trade.hpp"
#include "Types.hpp"
#include <cstdint>
#include <deque>
#include <functional>
#include <optional>
#include <vector>

using Price = uint64_t;

struct PriceLevel {
  Price price;
  std::deque<Order> orders;
};

class OrderBookFlatSorted {
public:
  OrderBookFlatSorted() = default;
  void add_order(Order &&order);
  std::optional<Order> remove_from_order(Side side, Price price);
  std::optional<std::reference_wrapper<const Order>> best_bid() const;
  std::optional<std::reference_wrapper<const Order>> best_ask() const;
  std::optional<std::reference_wrapper<const std::deque<Order>>> get_orders_at_price(Side side, Price price) const;
  bool is_empty(Side side) const;
  std::vector<Trade> processOrder(Order &&order);

private:
  std::vector<PriceLevel> bids_;
  std::vector<PriceLevel> asks_;
  uint64_t next_trade_id_ = 0;
};
