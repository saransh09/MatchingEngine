#pragma once

#include "Types.hpp"
#include <cstdint>

struct Trade {
  std::uint64_t trade_id;
  std::uint64_t buy_order_id;
  std::uint64_t sell_order_id;
  std::uint64_t price;
  std::uint64_t timestamp;
  std::uint32_t quantity;
  Side aggressor;
};

static_assert(sizeof(Trade) == 48, "Incompaible size of the Trade struct");
