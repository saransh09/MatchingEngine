#include "OrderGenerator.hpp"

OrderGenerator::OrderGenerator() : OrderGenerator(Config{}) {}

OrderGenerator::OrderGenerator(Config config)
    : config_(config), rng_(std::random_device{}()),
      price_dist_(config.min_price, config.max_price),
      quantity_dist_(config.min_quantity, config.max_quantity),
      side_dist_(config.buy_ratio) {}

Order OrderGenerator::generate() {
  Order order;
  order.order_id = next_order_id_++;
  order.timestamp = next_timestamp_++;
  order.price = price_dist_(rng_);
  order.quantity = quantity_dist_(rng_);
  order.remaining_quantity = order.quantity;
  order.side = side_dist_(rng_) ? Side::BUY : Side::SELL;
  order.order_type = OrderType::LIMIT;
  order.status = Status::NEW;
  order.symbol_id = 0;
  return order;
}

void OrderGenerator::reset() {
  next_order_id_ = 0;
  next_timestamp_ = 0;
}
