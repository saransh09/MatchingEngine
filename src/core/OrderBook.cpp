#include "OrderBook.hpp"
#include "Order.hpp"
#include <algorithm>
#include <deque>
#include <functional>
#include <optional>

void OrderBook::add_order(Order &&order) {
  if (order.side == Side::BUY) {
    bids[order.price].emplace_back(std::move(order));
  } else {
    asks[order.price].emplace_back(std::move(order));
  }
}

std::optional<Order> OrderBook::remove_from_order(Side side, Price price) {
  if (side == Side::BUY) {
    auto it = bids.find(price);
    if (it != bids.end() && !it->second.empty()) {
      auto curr = it->second.front();
      it->second.pop_front();
      if (it->second.empty()) {
        bids.erase(it);
      }
      return std::optional<Order>{curr};
    }
  } else {
    auto it = asks.find(price);
    if (it != asks.end() && !it->second.empty()) {
      auto curr = it->second.front();
      it->second.pop_front();
      if (it->second.empty()) {
        asks.erase(it);
      }
      return std::optional<Order>{curr};
    }
  }
  return std::optional<Order>{};
}

std::optional<std::reference_wrapper<const Order>> OrderBook::best_bid() const {
  if (!bids.empty()) {
    return std::make_optional(std::ref(bids.rbegin()->second.front()));
  }
  return std::optional<std::reference_wrapper<const Order>>{};
}

std::optional<std::reference_wrapper<const Order>> OrderBook::best_ask() const {
  if (!asks.empty()) {
    return std::make_optional(std::ref(asks.begin()->second.front()));
  }
  return std::optional<std::reference_wrapper<const Order>>{};
}

bool OrderBook::is_empty(Side side) const {
  if (side == Side::BUY) {
    return bids.empty();
  } else {
    return asks.empty();
  }
}

std::vector<Trade> OrderBook::processOrder(Order &&order) {
  std::vector<Trade> trades;

  if (order.side == Side::BUY) {
    while (order.remaining_quantity > 0) {
      auto best_ask = asks.begin();
      if (best_ask == asks.end()) {
        break;
      }
      auto &ask = best_ask->second.front();
      if (ask.price > order.price) {
        break;
      }

      auto quantity =
          std::min(order.remaining_quantity, ask.remaining_quantity);
      auto trade = Trade{
          .trade_id = next_trade_id++,
          .buy_order_id = order.order_id,
          .sell_order_id = ask.order_id,
          .price = ask.price,
          .timestamp = order.timestamp,
          .quantity = quantity,
          .aggressor = Side::BUY,
      };
      trades.push_back(trade);
      ask.remaining_quantity -= quantity;
      order.remaining_quantity -= quantity;
      if (ask.remaining_quantity == 0) {
        asks[ask.price].pop_front();
        if (asks[ask.price].empty()) {
          asks.erase(ask.price);
        }
      }
      if (order.remaining_quantity == 0) {
        break;
      }
    }
  } else {
    while (order.remaining_quantity > 0) {
      auto best_bid = bids.rbegin();
      if (best_bid == bids.rend()) {
        break;
      }
      auto &bid = best_bid->second.front();
      if (bid.price < order.price) {
        break;
      }
      auto quantity =
          std::min(order.remaining_quantity, bid.remaining_quantity);
      auto trade = Trade{.trade_id = next_trade_id++,
                         .buy_order_id = order.order_id,
                         .sell_order_id = bid.order_id,
                         .price = bid.price,
                         .timestamp = order.timestamp,
                         .quantity = quantity,
                         .aggressor = Side::SELL};
      trades.push_back(trade);
      bid.remaining_quantity -= quantity;
      order.remaining_quantity -= quantity;
      if (bid.remaining_quantity == 0) {
        bids[bid.price].pop_front();
        if (bids[bid.price].empty()) {
          bids.erase(bid.price);
        }
      }
      if (order.remaining_quantity == 0) {
        break;
      }
    }
  }
  if (order.remaining_quantity > 0) {
    add_order(std::move(order));
  }
  return trades;
}


std::optional<std::reference_wrapper<const std::deque<Order>>> OrderBook::get_orders_at_price(Side side, Price price) const {
    if (side == Side::BUY) {
        auto it = bids.find(price);
        if (it != bids.end()) {
            return std::ref(it->second);
        }
    } else {
        auto it = asks.find(price);
        if (it != asks.end()) {
            return std::ref(it->second);
        }
    }
    return std::optional<std::reference_wrapper<const std::deque<Order>>> {};
}
