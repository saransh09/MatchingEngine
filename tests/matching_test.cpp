#include "MatchingEngine.hpp"
#include "Order.hpp"
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

  book.add_order(std::move(order_1));
  auto buy_orders = book.get_orders_at_price(Side::BUY, 100);
  ASSERT_TRUE(buy_orders.has_value());
  ASSERT_EQ(buy_orders->get().front().order_id, 1);

  Order order_2{
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
  ASSERT_EQ(book.get_orders_at_price(Side::BUY, 100)
                ->get()
                .front()
                .remaining_quantity,
            5);
}

TEST_F(MatchingTest, PartialSellMatchTest) {
  Order order_1{
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

  Order order_2{
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
  ASSERT_EQ(book.get_orders_at_price(Side::SELL, 100)
                ->get()
                .front()
                .remaining_quantity,
            5);
}

TEST_F(MatchingTest, PartialFillRestingTest) {
  Order order_1{
      .order_id = 1,
      .timestamp = 0,
      .price = 100,
      .symbol_id = 0,
      .quantity = 5,
      .remaining_quantity = 5,
      .side = Side::SELL,
      .order_type = OrderType::LIMIT,
      .status = Status::NEW,
  };

  book.add_order(std::move(order_1));
  auto sell_orders = book.get_orders_at_price(Side::SELL, 100);
  ASSERT_TRUE(sell_orders.has_value());
  ASSERT_EQ(sell_orders->get().front().order_id, 1);

  Order order_2{
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
  ASSERT_EQ(book.get_orders_at_price(Side::BUY, 100)
                ->get()
                .front()
                .remaining_quantity,
            5);
}

TEST_F(MatchingTest, NoMatchBuyTooLowTest) {
  Order order_1{
      .order_id = 1,
      .timestamp = 0,
      .price = 100,
      .symbol_id = 0,
      .quantity = 5,
      .remaining_quantity = 5,
      .side = Side::SELL,
      .order_type = OrderType::LIMIT,
      .status = Status::NEW,
  };

  book.add_order(std::move(order_1));
  auto sell_orders = book.get_orders_at_price(Side::SELL, 100);
  ASSERT_TRUE(sell_orders.has_value());
  ASSERT_EQ(sell_orders->get().front().order_id, 1);

  Order order_2{
      .order_id = 2,
      .timestamp = 1,
      .price = 99,
      .symbol_id = 0,
      .quantity = 10,
      .remaining_quantity = 10,
      .side = Side::BUY,
      .order_type = OrderType::LIMIT,
      .status = Status::NEW,
  };

  auto trades = engine.processOrder(std::move(order_2));
  ASSERT_EQ(trades.size(), 0);
  ASSERT_EQ(book.get_orders_at_price(Side::SELL, 100)
                ->get()
                .front()
                .remaining_quantity,
            5);
  ASSERT_EQ(
      book.get_orders_at_price(Side::BUY, 99)->get().front().remaining_quantity,
      10);
}

TEST_F(MatchingTest, NoMatchSellTooHighTest) {
  Order order_1{
      .order_id = 1,
      .timestamp = 0,
      .price = 100,
      .symbol_id = 0,
      .quantity = 5,
      .remaining_quantity = 5,
      .side = Side::BUY,
      .order_type = OrderType::LIMIT,
      .status = Status::NEW,
  };

  book.add_order(std::move(order_1));
  auto sell_orders = book.get_orders_at_price(Side::BUY, 100);
  ASSERT_TRUE(sell_orders.has_value());
  ASSERT_EQ(sell_orders->get().front().order_id, 1);

  Order order_2{
      .order_id = 2,
      .timestamp = 1,
      .price = 101,
      .symbol_id = 0,
      .quantity = 10,
      .remaining_quantity = 10,
      .side = Side::SELL,
      .order_type = OrderType::LIMIT,
      .status = Status::NEW,
  };

  auto trades = engine.processOrder(std::move(order_2));
  ASSERT_EQ(trades.size(), 0);
  ASSERT_EQ(book.get_orders_at_price(Side::BUY, 100)
                ->get()
                .front()
                .remaining_quantity,
            5);
  ASSERT_EQ(book.get_orders_at_price(Side::SELL, 101)
                ->get()
                .front()
                .remaining_quantity,
            10);
}

TEST_F(MatchingTest, TimePriorityTest) {

  Order ask_a{
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

  Order ask_b{
      .order_id = 2,
      .timestamp = 1,
      .price = 100,
      .symbol_id = 0,
      .quantity = 10,
      .remaining_quantity = 10,
      .side = Side::SELL,
      .order_type = OrderType::LIMIT,
      .status = Status::NEW,
  };

  book.add_order(std::move(ask_a));
  book.add_order(std::move(ask_b));

  Order buy_order{
      .order_id = 3,
      .timestamp = 2,
      .price = 100,
      .symbol_id = 0,
      .quantity = 10,
      .remaining_quantity = 10,
      .side = Side::BUY,
      .order_type = OrderType::LIMIT,
      .status = Status::NEW,
  };

  auto trades = engine.processOrder(std::move(buy_order));

  ASSERT_EQ(trades.size(), 1);
  ASSERT_EQ(trades[0].sell_order_id, 1);

  auto remaining = book.get_orders_at_price(Side::SELL, 100);
  ASSERT_TRUE(remaining.has_value());
  ASSERT_EQ(remaining->get().front().order_id, 2);
}

TEST_F(MatchingTest, MultiFillTest) {
  Order ask_1{
      .order_id = 1,
      .timestamp = 0,
      .price = 1000,
      .symbol_id = 0,
      .quantity = 50,
      .remaining_quantity = 50,
      .side = Side::SELL,
      .order_type = OrderType::LIMIT,
      .status = Status::NEW,
  };

  Order ask_2{
      .order_id = 2,
      .timestamp = 1,
      .price = 1000,
      .symbol_id = 0,
      .quantity = 50,
      .remaining_quantity = 50,
      .side = Side::SELL,
      .order_type = OrderType::LIMIT,
      .status = Status::NEW,
  };

  Order ask_3{
      .order_id = 3,
      .timestamp = 2,
      .price = 1005,
      .symbol_id = 0,
      .quantity = 50,
      .remaining_quantity = 50,
      .side = Side::SELL,
      .order_type = OrderType::LIMIT,
      .status = Status::NEW,
  };

  book.add_order(std::move(ask_1));
  book.add_order(std::move(ask_2));
  book.add_order(std::move(ask_3));

  Order buy_order{
      .order_id = 4,
      .timestamp = 3,
      .price = 1005,
      .symbol_id = 0,
      .quantity = 120,
      .remaining_quantity = 120,
      .side = Side::BUY,
      .order_type = OrderType::LIMIT,
      .status = Status::NEW,
  };

  auto trades = engine.processOrder(std::move(buy_order));

  ASSERT_EQ(trades.size(), 3);
  ASSERT_EQ(trades[0].quantity, 50);
  ASSERT_EQ(trades[0].price, 1000);
  ASSERT_EQ(trades[1].quantity, 50);
  ASSERT_EQ(trades[1].price, 1000);
  ASSERT_EQ(trades[2].quantity, 20);
  ASSERT_EQ(trades[2].price, 1005);

  auto remaining = book.get_orders_at_price(Side::SELL, 1005);
  ASSERT_TRUE(remaining.has_value());
  ASSERT_EQ(remaining->get().front().remaining_quantity, 30);
}

TEST_F(MatchingTest, EmptyBookTest) {
  Order buy_order{
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

  auto trades = engine.processOrder(std::move(buy_order));

  ASSERT_EQ(trades.size(), 0);
  ASSERT_FALSE(book.is_empty(Side::BUY));

  auto best_bid = book.best_bid();
  ASSERT_TRUE(best_bid.has_value());
  ASSERT_EQ(best_bid->get().order_id, 1);
}

TEST_F(MatchingTest, PriceLevelCleanupTest) {
  Order ask{
      .order_id = 1,
      .timestamp = 0,
      .price = 1000,
      .symbol_id = 0,
      .quantity = 50,
      .remaining_quantity = 50,
      .side = Side::SELL,
      .order_type = OrderType::LIMIT,
      .status = Status::NEW,
  };

  book.add_order(std::move(ask));

  Order buy{
      .order_id = 2,
      .timestamp = 1,
      .price = 1000,
      .symbol_id = 0,
      .quantity = 50,
      .remaining_quantity = 50,
      .side = Side::BUY,
      .order_type = OrderType::LIMIT,
      .status = Status::NEW,
  };

  auto trades = engine.processOrder(std::move(buy));

  ASSERT_EQ(trades.size(), 1);
  ASSERT_TRUE(book.is_empty(Side::SELL));

  auto best_ask = book.best_ask();
  ASSERT_FALSE(best_ask.has_value());
}

TEST_F(MatchingTest, MultiplePriceLevelsTest) {
  Order bid_1{.order_id = 1,
              .timestamp = 0,
              .price = 1000,
              .symbol_id = 0,
              .quantity = 10,
              .remaining_quantity = 10,
              .side = Side::BUY,
              .order_type = OrderType::LIMIT,
              .status = Status::NEW};
  Order bid_2{.order_id = 2,
              .timestamp = 1,
              .price = 1005,
              .symbol_id = 0,
              .quantity = 10,
              .remaining_quantity = 10,
              .side = Side::BUY,
              .order_type = OrderType::LIMIT,
              .status = Status::NEW};
  Order bid_3{.order_id = 3,
              .timestamp = 2,
              .price = 1010,
              .symbol_id = 0,
              .quantity = 10,
              .remaining_quantity = 10,
              .side = Side::BUY,
              .order_type = OrderType::LIMIT,
              .status = Status::NEW};

  book.add_order(std::move(bid_1));
  book.add_order(std::move(bid_2));
  book.add_order(std::move(bid_3));

  auto best_bid = book.best_bid();
  ASSERT_EQ(best_bid->get().price, 1010);

  ASSERT_FALSE(book.is_empty(Side::BUY));
}

TEST_F(MatchingTest, StatusUpdatesTest) {
  Order ask{
      .order_id = 1,
      .timestamp = 0,
      .price = 1000,
      .symbol_id = 0,
      .quantity = 100,
      .remaining_quantity = 100,
      .side = Side::SELL,
      .order_type = OrderType::LIMIT,
      .status = Status::NEW,
  };

  book.add_order(std::move(ask));

  Order buy{
      .order_id = 2,
      .timestamp = 1,
      .price = 1000,
      .symbol_id = 0,
      .quantity = 50,
      .remaining_quantity = 50,
      .side = Side::BUY,
      .order_type = OrderType::LIMIT,
      .status = Status::NEW,
  };

  auto trades = engine.processOrder(std::move(buy));

  ASSERT_EQ(trades.size(), 1);
  ASSERT_EQ(trades[0].quantity, 50);

  auto remaining = book.get_orders_at_price(Side::SELL, 1000);
  ASSERT_TRUE(remaining.has_value());
  ASSERT_EQ(remaining->get().front().remaining_quantity, 50);
}

TEST_F(MatchingTest, StatusFilledTest) {
  Order ask{.order_id = 1,
            .timestamp = 0,
            .price = 1000,
            .symbol_id = 0,
            .quantity = 10,
            .remaining_quantity = 10,
            .side = Side::SELL,
            .order_type = OrderType::LIMIT,
            .status = Status::NEW};
  book.add_order(std::move(ask));

  Order buy{.order_id = 2,
            .timestamp = 1,
            .price = 1000,
            .symbol_id = 0,
            .quantity = 10,
            .remaining_quantity = 10,
            .side = Side::BUY,
            .order_type = OrderType::LIMIT,
            .status = Status::NEW};
  auto trades = engine.processOrder(std::move(buy));

  ASSERT_TRUE(book.is_empty(Side::SELL));
  ASSERT_TRUE(book.is_empty(Side::BUY));
}
TEST_F(MatchingTest, StatusPartiallyFilledTest) {
  Order ask{.order_id = 1,
            .timestamp = 0,
            .price = 1000,
            .symbol_id = 0,
            .quantity = 100,
            .remaining_quantity = 100,
            .side = Side::SELL,
            .order_type = OrderType::LIMIT,
            .status = Status::NEW};
  book.add_order(std::move(ask));

  Order buy{.order_id = 2,
            .timestamp = 1,
            .price = 1000,
            .symbol_id = 0,
            .quantity = 50,
            .remaining_quantity = 50,
            .side = Side::BUY,
            .order_type = OrderType::LIMIT,
            .status = Status::NEW};
  auto trades = engine.processOrder(std::move(buy));

  auto remaining = book.get_orders_at_price(Side::SELL, 1000);
  ASSERT_EQ(remaining->get().front().status, Status::PARTIALLY_FILLED);
  ASSERT_EQ(remaining->get().front().remaining_quantity, 50);
}
