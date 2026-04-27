#include "OrderBookFlatSorted.hpp"
#include "Order.hpp"
#include <algorithm>
#include <deque>
#include <functional>
#include <optional>

void OrderBookFlatSorted::add_order(Order &&order) {
  if (order.side == Side::BUY) {
    auto it = std::lower_bound(
        bids_.begin(), bids_.end(), order.price,
        [](const PriceLevel &pl, Price p) { return pl.price > p; });
    if (it == bids_.end() || it->price != order.price) {
      it = bids_.emplace(it, PriceLevel{order.price});
    }
    it->orders.emplace_back(std::move(order));
  } else {
    auto it = std::lower_bound(
        asks_.begin(), asks_.end(), order.price,
        [](const PriceLevel &pl, Price p) { return pl.price < p; });
    if (it == asks_.end() || it->price != order.price) {
      it = asks_.emplace(it, PriceLevel{order.price});
    }
    it->orders.emplace_back(std::move(order));
  }
}

std::optional<Order> OrderBookFlatSorted::remove_from_order(Side side,
                                                            Price price) {
  if (side == Side::BUY) {
    auto it = std::lower_bound(
        bids_.begin(), bids_.end(), price,
        [](const PriceLevel &pl, Price p) { return pl.price > p; });
    if (it != bids_.end() && it->price == price) {
      auto curr = it->orders.front();
      it->orders.pop_front();
      if (it->orders.empty()) {
        bids_.erase(it);
      }
      return std::optional<Order>{curr};
    }
  } else {
    auto it = std::lower_bound(
        asks_.begin(), asks_.end(), price,
        [](const PriceLevel &pl, Price p) { return pl.price < p; });
    if (it != asks_.end() && it->price == price) {
      auto curr = it->orders.front();
      it->orders.pop_front();
      if (it->orders.empty()) {
        asks_.erase(it);
      }
      return std::optional<Order>{curr};
    }
  }
  return std::optional<Order>{};
}

std::optional<std::reference_wrapper<const Order>>
OrderBookFlatSorted::best_bid() const {
  if (!bids_.empty()) {
    return std::make_optional(std::ref(bids_[0].orders.front()));
  }
  return std::optional<std::reference_wrapper<const Order>>{};
}

std::optional<std::reference_wrapper<const Order>>
OrderBookFlatSorted::best_ask() const {
  if (!asks_.empty()) {
    return std::make_optional(std::ref(asks_[0].orders.front()));
  }
  return std::optional<std::reference_wrapper<const Order>>{};
}

bool OrderBookFlatSorted::is_empty(Side side) const {
  if (side == Side::BUY) {
    return bids_.empty();
  } else {
    return asks_.empty();
  }
}

std::vector<Trade> OrderBookFlatSorted::processOrder(Order &&order) {
  std::vector<Trade> trades;
  trades.reserve(3);

  if (order.side == Side::BUY) {
    while (order.remaining_quantity > 0) {
      if (asks_.empty()) {
        break;
      }
      auto &best_ask = asks_[0].orders;
      if (best_ask.empty()) {
        break;
      }
      auto &ask = best_ask.front();
      if (ask.price > order.price) {
        break;
      }

      auto quantity =
          std::min(order.remaining_quantity, ask.remaining_quantity);
      auto trade = Trade{
          .trade_id = next_trade_id_++,
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
        ask.status = Status::FILLED;
        best_ask.pop_front();
        if (best_ask.empty()) {
          asks_.erase(asks_.begin());
        }
      } else {
        ask.status = Status::PARTIALLY_FILLED;
      }
      if (order.remaining_quantity == 0) {
        order.status = Status::FILLED;
        break;
      }
    }
  } else {
    while (order.remaining_quantity > 0) {
      if (bids_.empty()) {
        break;
      }
      auto &best_bid = bids_[0].orders;
      if (best_bid.empty()) {
        break;
      }
      auto &bid = best_bid.front();
      if (bid.price < order.price) {
        break;
      }
      auto quantity =
          std::min(order.remaining_quantity, bid.remaining_quantity);
      auto trade = Trade{.trade_id = next_trade_id_++,
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
        bid.status = Status::FILLED;
        best_bid.pop_front();
        if (best_bid.empty()) {
          bids_.erase(bids_.begin());
        }
      } else {
        bid.status = Status::PARTIALLY_FILLED;
      }
      if (order.remaining_quantity == 0) {
        order.status = Status::FILLED;
        break;
      }
    }
  }
  if (order.remaining_quantity > 0) {
    order.status = Status::PARTIALLY_FILLED;
    add_order(std::move(order));
  }
  return trades;
}

std::optional<std::reference_wrapper<const std::deque<Order>>>
OrderBookFlatSorted::get_orders_at_price(Side side, Price price) const {
  if (side == Side::BUY) {
    auto it = std::lower_bound(
        bids_.begin(), bids_.end(), price,
        [](const PriceLevel &pl, Price p) { return pl.price > p; });
    if (it != bids_.end() && it->price == price) {
      return std::make_optional(std::ref(it->orders));
    }
  } else {
    auto it = std::lower_bound(
        asks_.begin(), asks_.end(), price,
        [](const PriceLevel &pl, Price p) { return pl.price < p; });
    if (it != asks_.end() && it->price == price) {
      return std::make_optional(std::ref(it->orders));
    }
  }
  return std::optional<std::reference_wrapper<const std::deque<Order>>>{};
}
