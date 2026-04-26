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

  if (config_.enable_clustering) {
      order.price = config_.clustering_price + (current_cluster_index_ * 500);
      orders_in_current_cluster_++;
      if (orders_in_current_cluster_ >= config_.cluster_count) {
          orders_in_current_cluster_ = 0;
          current_cluster_index_ = (current_cluster_index_ + 1) % config_.num_clusters;
      }
  }
  else {
      order.price = price_dist_(rng_);
  }
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
  orders_in_current_cluster_ = 0;
  current_cluster_index_ = 0;
}
