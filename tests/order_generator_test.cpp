#include "OrderGenerator.hpp"
#include <gtest/gtest.h>

class OrderGeneratorTest : public ::testing::Test {
protected:
  OrderGenerator generator_;
};

TEST_F(OrderGeneratorTest, GenerateValidOrder) {
  OrderGenerator generator;

  Order order = generator.generate();

  ASSERT_GE(order.order_id, 0);
  ASSERT_GE(order.timestamp, 0);
  ASSERT_GT(order.price, 0);
  ASSERT_GT(order.quantity, 0);
  ASSERT_EQ(order.quantity, order.remaining_quantity);
  ASSERT_TRUE(order.side == Side::BUY || order.side == Side::SELL);
}

TEST_F(OrderGeneratorTest, MonotonicIds) {
    OrderGenerator generator;

    Order order1 = generator.generate();
    Order order2 = generator.generate();
    Order order3 = generator.generate();

    ASSERT_EQ(order2.order_id, order1.order_id + 1);
    ASSERT_EQ(order3.order_id, order2.order_id + 1);
}
