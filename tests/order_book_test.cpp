#include <gtest/gtest.h>
#include "Order.hpp"
#include "OrderBook.hpp"


class OrderBookTest : public testing::Test {
    protected:
        OrderBook order_book;
};

TEST_F(OrderBookTest, AddBuyOrderTest) {
    Order ord {
        .order_id = 1,
        .timestamp = 123,
        .price = 100,
        .symbol_id = 0,
        .quantity = 10,
        .remaining_quantity = 10,
        .side = Side::BUY,
        .order_type = OrderType::MARKET,
        .status = Status::NEW,
    };
    order_book.add_order(std::move(ord));
    ASSERT_FALSE(order_book.is_empty(Side::BUY));
    ASSERT_TRUE(order_book.is_empty(Side::SELL));
}

TEST_F(OrderBookTest, AddSellOrderTest) {
    Order ord {
        .order_id = 1,
        .timestamp = 123,
        .price = 100,
        .symbol_id = 0,
        .quantity = 10,
        .remaining_quantity = 10,
        .side = Side::SELL,
        .order_type = OrderType::MARKET,
        .status = Status::NEW,
    };
    order_book.add_order(std::move(ord));
    ASSERT_FALSE(order_book.is_empty(Side::SELL));
    ASSERT_TRUE(order_book.is_empty(Side::BUY));
}

TEST_F(OrderBookTest, BestBidTest) {
    Order ord_1 {
        .order_id = 1,
        .timestamp = 123,
        .price = 100,
        .symbol_id = 0,
        .quantity = 10,
        .remaining_quantity = 10,
        .side = Side::BUY,
        .order_type = OrderType::MARKET,
        .status = Status::NEW,
    };
    Order ord_2 {
        .order_id = 2,
        .timestamp = 124,
        .price = 110,
        .symbol_id = 0,
        .quantity = 20,
        .remaining_quantity = 20,
        .side = Side::BUY,
        .order_type = OrderType::MARKET,
        .status = Status::NEW,
    };
    Order ord_3 {
        .order_id = 3,
        .timestamp = 125,
        .price = 90,
        .symbol_id = 0,
        .quantity = 20,
        .remaining_quantity = 20,
        .side = Side::BUY,
        .order_type = OrderType::MARKET,
        .status = Status::NEW,
    };
    order_book.add_order(std::move(ord_1));
    order_book.add_order(std::move(ord_2));
    order_book.add_order(std::move(ord_3));
    auto best_bid = order_book.best_bid();
    ASSERT_TRUE(best_bid.has_value());
    ASSERT_EQ(best_bid->get().order_id, 2);
}

TEST_F(OrderBookTest, BestAskTest) {
    Order ord_1 {
        .order_id = 1,
        .timestamp = 123,
        .price = 100,
        .symbol_id = 0,
        .quantity = 10,
        .remaining_quantity = 10,
        .side = Side::SELL,
        .order_type = OrderType::MARKET,
        .status = Status::NEW,
    };
    Order ord_2 {
        .order_id = 2,
        .timestamp = 124,
        .price = 110,
        .symbol_id = 0,
        .quantity = 20,
        .remaining_quantity = 20,
        .side = Side::SELL,
        .order_type = OrderType::MARKET,
        .status = Status::NEW,
    };
    Order ord_3 {
        .order_id = 3,
        .timestamp = 125,
        .price = 90,
        .symbol_id = 0,
        .quantity = 20,
        .remaining_quantity = 20,
        .side = Side::SELL,
        .order_type = OrderType::MARKET,
        .status = Status::NEW,
    };
    order_book.add_order(std::move(ord_1));
    order_book.add_order(std::move(ord_2));
    order_book.add_order(std::move(ord_3));
    auto best_ask = order_book.best_ask();
    ASSERT_TRUE(best_ask.has_value());
    ASSERT_EQ(best_ask->get().order_id, 3);
}

TEST_F(OrderBookTest, RemoveOrderBuy) {
    Order ord_1 {
        .order_id = 1,
        .timestamp = 123,
        .price = 100,
        .symbol_id = 0,
        .quantity = 10,
        .remaining_quantity = 10,
        .side = Side::BUY,
        .order_type = OrderType::MARKET,
        .status = Status::NEW,
    };
    order_book.add_order(std::move(ord_1));
    auto buy_order = order_book.remove_from_order(Side::BUY, 100);
    ASSERT_TRUE(buy_order.has_value());
    ASSERT_EQ(buy_order->order_id, 1);
    ASSERT_TRUE(order_book.is_empty(Side::BUY));
}

TEST_F(OrderBookTest, RemoveOrderBuyEmpty) {
    auto buy_order = order_book.remove_from_order(Side::BUY, 100);
    ASSERT_FALSE(buy_order.has_value());
}

TEST_F(OrderBookTest, RemoveOrderSell) {
    Order ord_1 {
        .order_id = 1,
        .timestamp = 123,
        .price = 100,
        .symbol_id = 0,
        .quantity = 10,
        .remaining_quantity = 10,
        .side = Side::SELL,
        .order_type = OrderType::MARKET,
        .status = Status::NEW,
    };
    order_book.add_order(std::move(ord_1));
    auto sell_order = order_book.remove_from_order(Side::SELL, 100);
    ASSERT_TRUE(sell_order.has_value());
    ASSERT_EQ(sell_order->order_id, 1);
    ASSERT_TRUE(order_book.is_empty(Side::SELL));
}

TEST_F(OrderBookTest, RemoveOrderSellEmpty) {
    auto sell_order = order_book.remove_from_order(Side::SELL, 100);
    ASSERT_FALSE(sell_order.has_value());
}

TEST_F(OrderBookTest, RemoveOrderCleansLevel) {
    Order ord {
        .order_id = 1,
        .timestamp = 123,
        .price = 100,
        .symbol_id = 0,
        .quantity = 10,
        .remaining_quantity = 10,
        .side = Side::BUY,
        .order_type = OrderType::LIMIT,
        .status = Status::NEW,
    };
    order_book.add_order(std::move(ord));
    ASSERT_FALSE(order_book.is_empty(Side::BUY));

    auto removed = order_book.remove_from_order(Side::BUY, 100);
    ASSERT_TRUE(removed.has_value());
    ASSERT_TRUE(order_book.is_empty(Side::BUY));

    auto best_bid = order_book.best_bid();
    ASSERT_FALSE(best_bid.has_value());
}
