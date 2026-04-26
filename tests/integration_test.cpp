#include "OrderBook.hpp"
#include <gtest/gtest.h>

TEST(IntegrationTest, MultiOrderFlow) {
  OrderBook book;
  std::vector<Trade> all_trades;

  book.add_order({.order_id = 1,
                  .timestamp = 0,
                  .price = 100000,
                  .symbol_id = 0,
                  .quantity = 100,
                  .remaining_quantity = 100,
                  .side = Side::SELL,
                  .order_type = OrderType::LIMIT,
                  .status = Status::NEW});
  book.add_order({.order_id = 2,
                  .timestamp = 0,
                  .price = 100500,
                  .symbol_id = 0,
                  .quantity = 50,
                  .remaining_quantity = 50,
                  .side = Side::SELL,
                  .order_type = OrderType::LIMIT,
                  .status = Status::NEW});
  book.add_order({.order_id = 3,
                  .timestamp = 0,
                  .price = 101000,
                  .symbol_id = 0,
                  .quantity = 75,
                  .remaining_quantity = 75,
                  .side = Side::SELL,
                  .order_type = OrderType::LIMIT,
                  .status = Status::NEW});

  auto trades = book.processOrder({.order_id = 4,
                                   .timestamp = 0,
                                   .price = 101000,
                                   .symbol_id = 0,
                                   .quantity = 150,
                                   .remaining_quantity = 150,
                                   .side = Side::BUY,
                                   .order_type = OrderType::LIMIT,
                                   .status = Status::NEW});

  all_trades.insert(all_trades.end(), trades.begin(), trades.end());

  ASSERT_EQ(all_trades.size(), 2);

  ASSERT_EQ(all_trades[0].price, 100000);
  ASSERT_EQ(all_trades[1].price, 100500);

  ASSERT_EQ(all_trades[0].quantity, 100);
  ASSERT_EQ(all_trades[1].quantity, 50);

  auto best_ask = book.best_ask();
  ASSERT_TRUE(best_ask.has_value());
  ASSERT_EQ(best_ask->get().order_id, 3);
  ASSERT_EQ(best_ask->get().remaining_quantity, 75);
}

TEST(IntegrationTest, MixedMatchTypes) {
  OrderBook book;
  std::vector<Trade> all_trades;

  book.add_order({.order_id = 1,
                  .timestamp = 0,
                  .price = 100000,
                  .symbol_id = 0,
                  .quantity = 100,
                  .remaining_quantity = 100,
                  .side = Side::SELL,
                  .order_type = OrderType::LIMIT,
                  .status = Status::NEW});

  auto trades = book.processOrder({.order_id = 2,
                                   .timestamp = 0,
                                   .price = 100000,
                                   .symbol_id = 0,
                                   .quantity = 100,
                                   .remaining_quantity = 100,
                                   .side = Side::BUY,
                                   .order_type = OrderType::LIMIT,
                                   .status = Status::NEW});
  all_trades.insert(all_trades.end(), trades.begin(), trades.end());

  book.add_order({.order_id = 3,
                  .timestamp = 0,
                  .price = 100000,
                  .symbol_id = 0,
                  .quantity = 100,
                  .remaining_quantity = 100,
                  .side = Side::SELL,
                  .order_type = OrderType::LIMIT,
                  .status = Status::NEW});

  trades = book.processOrder({.order_id = 4,
                              .timestamp = 0,
                              .price = 100000,
                              .symbol_id = 0,
                              .quantity = 50,
                              .remaining_quantity = 50,
                              .side = Side::BUY,
                              .order_type = OrderType::LIMIT,
                              .status = Status::NEW});
  all_trades.insert(all_trades.end(), trades.begin(), trades.end());

  trades = book.processOrder({.order_id = 5,
                              .timestamp = 0,
                              .price = 90000,
                              .symbol_id = 0,
                              .quantity = 100,
                              .remaining_quantity = 100,
                              .side = Side::BUY,
                              .order_type = OrderType::LIMIT,
                              .status = Status::NEW});
  all_trades.insert(all_trades.end(), trades.begin(), trades.end());

  ASSERT_EQ(all_trades.size(), 2);

  ASSERT_EQ(all_trades[0].quantity, 100);
  ASSERT_EQ(all_trades[0].price, 100000);

  ASSERT_EQ(all_trades[1].quantity, 50);
  ASSERT_EQ(all_trades[1].price, 100000);

  auto remaining_sell = book.get_orders_at_price(Side::SELL, 100000);
  ASSERT_TRUE(remaining_sell.has_value());
  ASSERT_EQ(remaining_sell->get().front().remaining_quantity, 50);
  ASSERT_EQ(remaining_sell->get().front().status, Status::PARTIALLY_FILLED);

  auto best_bid = book.best_bid();
  ASSERT_TRUE(best_bid.has_value());
  ASSERT_EQ(best_bid->get().price, 90000);
  ASSERT_EQ(best_bid->get().order_id, 5);
  ASSERT_EQ(best_bid->get().status, Status::PARTIALLY_FILLED);
}

TEST(IntegrationTest, OrderStatusLifecycle) {
  OrderBook book;

  book.add_order({.order_id = 1,
                  .timestamp = 0,
                  .price = 100000,
                  .symbol_id = 0,
                  .quantity = 100,
                  .remaining_quantity = 100,
                  .side = Side::SELL,
                  .order_type = OrderType::LIMIT,
                  .status = Status::NEW});

  auto trades = book.processOrder({.order_id = 2,
                                   .timestamp = 0,
                                   .price = 100000,
                                   .symbol_id = 0,
                                   .quantity = 50,
                                   .remaining_quantity = 50,
                                   .side = Side::BUY,
                                   .order_type = OrderType::LIMIT,
                                   .status = Status::NEW});

  ASSERT_EQ(trades.size(), 1);
  ASSERT_EQ(trades[0].quantity, 50);

  auto remaining = book.best_ask();
  ASSERT_TRUE(remaining.has_value());
  ASSERT_EQ(remaining->get().status, Status::PARTIALLY_FILLED);
  ASSERT_EQ(remaining->get().remaining_quantity, 50);

  auto trades2 = book.processOrder({.order_id = 3,
                                    .timestamp = 0,
                                    .price = 100000,
                                    .symbol_id = 0,
                                    .quantity = 50,
                                    .remaining_quantity = 50,
                                    .side = Side::BUY,
                                    .order_type = OrderType::LIMIT,
                                    .status = Status::NEW});

  ASSERT_TRUE(book.is_empty(Side::SELL));

  ASSERT_EQ(trades2.size(), 1);
  ASSERT_EQ(trades2[0].quantity, 50);
}

TEST(IntegrationTest, SweepMultipleLevels) {
  OrderBook book;
  std::vector<Trade> all_trades;

  book.add_order({.order_id = 1,
                  .timestamp = 0,
                  .price = 100000,
                  .symbol_id = 0,
                  .quantity = 50,
                  .remaining_quantity = 50,
                  .side = Side::SELL,
                  .order_type = OrderType::LIMIT,
                  .status = Status::NEW});

  book.add_order({.order_id = 2,
                  .timestamp = 0,
                  .price = 100500,
                  .symbol_id = 0,
                  .quantity = 50,
                  .remaining_quantity = 50,
                  .side = Side::SELL,
                  .order_type = OrderType::LIMIT,
                  .status = Status::NEW});

  book.add_order({.order_id = 3,
                  .timestamp = 0,
                  .price = 101000,
                  .symbol_id = 0,
                  .quantity = 50,
                  .remaining_quantity = 50,
                  .side = Side::SELL,
                  .order_type = OrderType::LIMIT,
                  .status = Status::NEW});

  auto trades = book.processOrder({.order_id = 4,
                                   .timestamp = 0,
                                   .price = 101000,
                                   .symbol_id = 0,
                                   .quantity = 120,
                                   .remaining_quantity = 120,
                                   .side = Side::BUY,
                                   .order_type = OrderType::LIMIT,
                                   .status = Status::NEW});
  all_trades.insert(all_trades.end(), trades.begin(), trades.end());

  ASSERT_EQ(all_trades.size(), 3);

  ASSERT_EQ(all_trades[0].price, 100000);
  ASSERT_EQ(all_trades[0].quantity, 50);

  ASSERT_EQ(all_trades[1].price, 100500);
  ASSERT_EQ(all_trades[1].quantity, 50);

  ASSERT_EQ(all_trades[2].price, 101000);
  ASSERT_EQ(all_trades[2].quantity, 20);

  auto remaining = book.best_ask();
  ASSERT_TRUE(remaining.has_value());
  ASSERT_EQ(remaining->get().order_id, 3);
  ASSERT_EQ(remaining->get().remaining_quantity, 30);
  ASSERT_EQ(remaining->get().status, Status::PARTIALLY_FILLED);
}

TEST(IntegrationTest, TradeDetailsVerification) {
  OrderBook book;

  book.add_order({.order_id = 1,
                  .timestamp = 0,
                  .price = 100000,
                  .symbol_id = 0,
                  .quantity = 100,
                  .remaining_quantity = 100,
                  .side = Side::SELL,
                  .order_type = OrderType::LIMIT,
                  .status = Status::NEW});

  auto trades = book.processOrder({.order_id = 2,
                                   .timestamp = 0,
                                   .price = 100000,
                                   .symbol_id = 0,
                                   .quantity = 50,
                                   .remaining_quantity = 50,
                                   .side = Side::BUY,
                                   .order_type = OrderType::LIMIT,
                                   .status = Status::NEW});

  ASSERT_EQ(trades.size(), 1);

  ASSERT_EQ(trades[0].buy_order_id, 2);

  ASSERT_EQ(trades[0].sell_order_id, 1);

  ASSERT_EQ(trades[0].price, 100000);

  ASSERT_EQ(trades[0].quantity, 50);

  ASSERT_EQ(trades[0].aggressor, Side::BUY);

  ASSERT_GE(trades[0].timestamp, 0);

  ASSERT_GE(trades[0].trade_id, 0);
}
