#include <benchmark/benchmark.h>
#include <utility>
#include "OrderGenerator.hpp"
#include "OrderBook.hpp"

static void BM_ProcessOrder_EmptyBook(benchmark::State& state) {
    OrderGenerator generator;
    OrderBook book;
    for (auto _ : state) {
        Order order = generator.generate();
        auto trades = book.processOrder(std::move(order));
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(trades);
    }
}
BENCHMARK(BM_ProcessOrder_EmptyBook);

static void BM_ProcessOrder_WithRestingOrders(benchmark::State& state) {
    OrderGenerator generator;
    OrderBook book;

    for (size_t i = 0 ; i < 100 ; ++i) {
        book.add_order(std::forward<Order>(generator.generate()));
    }
    for (auto _ : state) {
        book.processOrder(std::forward<Order>(generator.generate()));
    }
}
BENCHMARK(BM_ProcessOrder_WithRestingOrders);

static void BM_AddOrder(benchmark::State& state) {
    OrderGenerator generator;
    OrderBook book;
    for (auto _ : state) {
        book.add_order(generator.generate());
        benchmark::DoNotOptimize(book);
    }
}
BENCHMARK(BM_AddOrder);

static void BM_BestBid(benchmark::State& state) {
    OrderGenerator generator;
    OrderBook book;
    for (int i = 0; i < 100; ++i) book.add_order(generator.generate());

    for (auto _ : state) {
        auto bid = book.best_bid();
        benchmark::DoNotOptimize(bid);
    }
}
BENCHMARK(BM_BestBid);

static void BM_BestAsk(benchmark::State& state) {
    OrderGenerator generator;
    OrderBook book;
    for (int i = 0; i < 100; ++i) book.add_order(generator.generate());
    for (auto _ : state) {
        auto ask = book.best_ask();
        benchmark::DoNotOptimize(ask);
    }
}
BENCHMARK(BM_BestAsk);

static void BM_IsEmpty(benchmark::State& state) {
    OrderGenerator generator;
    OrderBook book;
    for (int i = 0; i < 100; ++i) book.add_order(generator.generate());
    for (auto _ : state) {
        auto empty = book.is_empty(Side::BUY);
        benchmark::DoNotOptimize(empty);
    }
}
BENCHMARK(BM_IsEmpty);

static void BM_RemoveOrder(benchmark::State& state) {
    OrderGenerator generator;
    OrderBook book;
    for (int i = 0; i < 100; ++i) book.add_order(generator.generate());
    for (auto _ : state) {
        auto removed = book.remove_from_order(Side::BUY, 100000);
        benchmark::DoNotOptimize(removed);
        book.add_order(generator.generate());
    }
}
BENCHMARK(BM_RemoveOrder);

static void BM_GetOrdersAtPrice(benchmark::State& state) {
    OrderGenerator generator;
    OrderBook book;
    for (int i = 0; i < 100; ++i) book.add_order(generator.generate());
    for (auto _ : state) {
        auto orders = book.get_orders_at_price(Side::BUY, 100000);
        benchmark::DoNotOptimize(orders);
    }
}
BENCHMARK(BM_GetOrdersAtPrice);


static void BM_ProcessOrder_ExactMatch(benchmark::State& state) {
    OrderGenerator generator;
    OrderBook book;

    Order ask{1, 0, 100000, 0, 10, 10, Side::SELL, OrderType::LIMIT, Status::NEW};
    book.add_order(std::move(ask));

    for (auto _ : state) {
        Order buy{2, 1, 100000, 0, 10, 10, Side::BUY, OrderType::LIMIT, Status::NEW};
        auto trades = book.processOrder(std::move(buy));
        benchmark::DoNotOptimize(trades);
        book.add_order(Order{1, 0, 100000, 0, 10, 10, Side::SELL, OrderType::LIMIT, Status::NEW});
    }
}
BENCHMARK(BM_ProcessOrder_ExactMatch);

static void BM_ProcessOrder_PartialMatch(benchmark::State& state) {
    OrderGenerator generator;
    OrderBook book;

    Order ask{1, 0, 100000, 0, 100, 100, Side::SELL, OrderType::LIMIT, Status::NEW};
    book.add_order(std::move(ask));

    for (auto _ : state) {
        Order buy{2, 1, 100000, 0, 10, 10, Side::BUY, OrderType::LIMIT, Status::NEW};
        auto trades = book.processOrder(std::move(buy));
        benchmark::DoNotOptimize(trades);
        book.add_order(Order{1, 0, 100000, 0, 100, 100, Side::SELL, OrderType::LIMIT, Status::NEW});
    }
}
BENCHMARK(BM_ProcessOrder_PartialMatch);

static void BM_ProcessOrder_NoMatch(benchmark::State& state) {
    OrderGenerator generator;
    OrderBook book;

    Order ask{1, 0, 100000, 0, 10, 10, Side::SELL, OrderType::LIMIT, Status::NEW};
    book.add_order(std::move(ask));

    for (auto _ : state) {
        Order buy{2, 1, 90000, 0, 10, 10, Side::BUY, OrderType::LIMIT, Status::NEW};
        auto trades = book.processOrder(std::move(buy));
        benchmark::DoNotOptimize(trades);
    }
}
BENCHMARK(BM_ProcessOrder_NoMatch);

// -----------------------------------------------
// Clustered benchmark for more deque operation
// -----------------------------------------------
static void BM_AddOrder_Clustered(benchmark::State& state) {
    OrderGenerator generator({
        .min_price = 100000,
        .max_price = 110000,
        .min_quantity = 10,
        .max_quantity = 50,
        .buy_ratio = 0.5,
        .enable_clustering = true,
        .clustering_price = 100000,
        .cluster_count = 1000,
        .num_clusters = 3
    });
    OrderBook book;

    for (auto _ : state) {
        Order order = generator.generate();
        book.add_order(std::move(order));
    }
}
BENCHMARK(BM_AddOrder_Clustered);

static void BM_ProcessOrder_Clustered(benchmark::State& state) {
    OrderGenerator generator({
        .min_price = 100000,
        .max_price = 110000,
        .min_quantity = 10,
        .max_quantity = 50,
        .buy_ratio = 0.5,
        .enable_clustering = true,
        .clustering_price = 100000,
        .cluster_count = 1000,
        .num_clusters = 3
    });
    OrderBook book;

    for (int i = 0; i < 1000; ++i) {
        book.add_order(generator.generate());
    }

    for (auto _ : state) {
        auto trades = book.processOrder(generator.generate());
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(trades);
    }
}
BENCHMARK(BM_ProcessOrder_Clustered);

static void BM_AddOrder_HighClustering(benchmark::State& state) {
    OrderGenerator generator({
        .min_price = 100000,
        .max_price = 110000,
        .min_quantity = 10,
        .max_quantity = 50,
        .buy_ratio = 0.5,
        .enable_clustering = true,
        .clustering_price = 100000,
        .cluster_count = 10000,
        .num_clusters = 1
    });
    OrderBook book;

    for (auto _ : state) {
        book.add_order(generator.generate());
        benchmark::DoNotOptimize(book);
    }
}
BENCHMARK(BM_AddOrder_HighClustering);


BENCHMARK_MAIN();
