#pragma once

#include <cstdint>

enum class Side : std::uint8_t { BUY, SELL };

enum class OrderType : std::uint8_t { LIMIT, MARKET, IOC, FOK };

enum class Status : std::uint8_t { NEW, PARTIALLY_FILLED, FILLED, CANCELLED };
