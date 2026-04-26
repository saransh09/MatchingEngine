#pragma once

#include "Order.hpp"
#include <cstdint>
#include <random>

class OrderGenerator {
public:
  struct Config {
    uint64_t min_price = 50000;
    uint64_t max_price = 500000;
    uint64_t min_quantity = 1;
    uint64_t max_quantity = 100;
    double buy_ratio = 0.5;
  };

  OrderGenerator();
  explicit OrderGenerator(Config config);

  Order generate();
  void reset();

private:
  Config config_;
  std::mt19937_64 rng_;
  std::uniform_int_distribution<uint64_t> price_dist_;
  std::uniform_int_distribution<uint64_t> quantity_dist_;
  std::bernoulli_distribution side_dist_;

  uint64_t next_order_id_ = 0;
  uint64_t next_timestamp_ = 0;
};
