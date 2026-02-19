#pragma once

#include <cstdint>
#include "Types.hpp"

struct Order {

  std::uint64_t order_id;
  std::uint64_t timestamp;
  std::uint64_t price;
  std::uint32_t symbol_id;
  std::uint32_t quantity;
  std::uint32_t remaining_quantity;
  Side side;
  OrderType order_type;
  Status status;
};

static_assert(sizeof(Order) == 40, "Incompatible size");
