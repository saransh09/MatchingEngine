#include "MatchingEngine.hpp"
#include "Order.hpp"
#include "Trade.hpp"
#include <gtest/gtest.h>
#include <utility>

class MatchingTest : public testing::Test {
protected:
  OrderBook book;
  MatchingEngine engine{book};
};

TEST_F(MatchingTest, ExactBuyMatchTest) {
  Order order_1{
      .order_id = 1,
      .timestamp = 0,
      .price = 100,
      .symbol_id = 0,
      .quantity = 10,
      .remaining_quantity = 10,
      .side = Side::BUY,
      .order_type = OrderType::LIMIT,
      .status = Status::NEW,
  };

  Order order_2{
      .order_id = 2,
      .timestamp = 0,
      .price = 100,
      .symbol_id = 0,
      .quantity = 10,
      .remaining_quantity = 10,
      .side = Side::SELL,
      .order_type = OrderType::LIMIT,
      .status = Status::NEW,
  };

  book.add_order(std::move(order_1));
  ASSERT_FALSE(book.is_empty(Side::BUY));

  auto trades = engine.processOrder(std::move(order_2));

  ASSERT_TRUE(book.is_empty(Side::BUY));
  ASSERT_EQ(trades.size(), 1);
}

TEST_F(MatchingTest, ExactSellMatchTest) {
  Order order_1{
      .order_id = 1,
      .timestamp = 0,
      .price = 100,
      .symbol_id = 0,
      .quantity = 10,
      .remaining_quantity = 10,
      .side = Side::SELL,
      .order_type = OrderType::LIMIT,
      .status = Status::NEW,
  };

  Order order_2{
      .order_id = 2,
      .timestamp = 0,
      .price = 100,
      .symbol_id = 0,
      .quantity = 10,
      .remaining_quantity = 10,
      .side = Side::BUY,
      .order_type = OrderType::LIMIT,
      .status = Status::NEW,
  };

  book.add_order(std::move(order_1));

  ASSERT_FALSE(book.is_empty(Side::SELL));

  auto trades = engine.processOrder(std::move(order_2));

  ASSERT_TRUE(book.is_empty(Side::SELL));
  ASSERT_EQ(trades.size(), 1);
}


TEST_F(MatchingTest, PartialBuyMatchTest) {
    Order order_1 {
        .order_id = 1,
        .timestamp = 0,
        .price = 100,
        .symbol_id = 0,
        .quantity = 10,
        .remaining_quantity = 10,
        .side = Side::BUY,
        .order_type = OrderType::LIMIT,
        .status = Status::NEW,
    };

    book.add_order(std::move(order_1));
    auto buy_orders = book.get_orders_at_price(Side::BUY, 100);
    ASSERT_TRUE(buy_orders.has_value());
    ASSERT_EQ(buy_orders->get().front().order_id, 1);

    Order order_2 {
        .order_id = 2,
        .timestamp = 1,
        .price = 100,
        .symbol_id = 0,
        .quantity = 5,
        .remaining_quantity = 5,
        .side = Side::SELL,
        .order_type = OrderType::LIMIT,
        .status = Status::NEW,
    };

    auto trades = engine.processOrder(std::move(order_2));
    ASSERT_EQ(trades.size(), 1);
    ASSERT_EQ(trades[0].quantity, 5);
    ASSERT_EQ(book.get_orders_at_price(Side::BUY, 100)->get().front().remaining_quantity, 5);
}

TEST_F(MatchingTest, PartialSellMatchTest) {
    Order order_1 {
        .order_id = 1,
        .timestamp = 0,
        .price = 100,
        .symbol_id = 0,
        .quantity = 15,
        .remaining_quantity = 15,
        .side = Side::SELL,
        .order_type = OrderType::LIMIT,
        .status = Status::NEW,
    };

    book.add_order(std::move(order_1));
    auto sell_orders = book.get_orders_at_price(Side::SELL, 100);
    ASSERT_TRUE(sell_orders.has_value());
    ASSERT_EQ(sell_orders->get().front().order_id, 1);

    Order order_2 {
        .order_id = 2,
        .timestamp = 1,
        .price = 100,
        .symbol_id = 0,
        .quantity = 10,
        .remaining_quantity = 10,
        .side = Side::BUY,
        .order_type = OrderType::LIMIT,
        .status = Status::NEW,
    };

    auto trades = engine.processOrder(std::move(order_2));
    ASSERT_EQ(book.get_orders_at_price(Side::SELL, 100)->get().front().remaining_quantity, 5);
}
