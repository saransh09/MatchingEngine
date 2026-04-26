# Benchmark Results

## Overview

This document tracks the performance baseline of the Matching Engine. All benchmarks measure individual operations to understand the performance characteristics of our order book implementation.

**Date:** 2026-04-26  
**Environment:** macOS, Apple Silicon M2 (10 cores @ 24 MHz)  
**Build:** Release mode with optimizations enabled

---

## Quick Summary Table

| Benchmark                         | Time (ns)  | Throughput (ops/sec) | Category                 |
| --------------------------------- | ---------- | -------------------- | ------------------------ |
| BM_IsEmpty                        | 0.86       | 1.16B/sec            | Query                    |
| BM_BestAsk                        | 1.16       | 870M/sec             | Query                    |
| BM_BestBid                        | 3.18       | 315M/sec             | Query                    |
| BM_GetOrdersAtPrice               | 4.45       | 225M/sec             | Query                    |
| BM_ProcessOrder_NoMatch           | 7.52       | 133M/sec             | Matching                 |
| BM_ProcessOrder_PartialMatch      | 26.88      | 37M/sec              | Matching                 |
| BM_ProcessOrder_ExactMatch        | 81.36      | 12M/sec              | Matching                 |
| BM_ProcessOrder_WithRestingOrders | 185.61     | 5.4M/sec             | Matching                 |
| BM_ProcessOrder_EmptyBook         | 200.22     | 5.0M/sec             | Matching                 |
| BM_RemoveOrder                    | 577.07     | 1.7M/sec             | Mutation                 |
| BM_AddOrder                       | 853.94     | 1.17M/sec            | Mutation                 |
| **BM_AddOrder_HighClustering**    | **33.85**  | **29.6M/sec**        | **Clustered**            |
| **BM_AddOrder_Clustered**         | **34.90**  | **28.7M/sec**        | **Clustered**            |
| **BM_ProcessOrder_Clustered**     | **57.23**  | **17.5M/sec**        | **Clustered**            |
| **BM_Integration_Extreme**        | **154.08** | **6.5M/sec**         | **Extreme (10K orders)** |

---

## Detailed Results

### Query Operations (< 5 ns)

These operations are extremely fast because they involve simple map lookups:

| Operation           | Time (ns) | Throughput | Notes                        |
| ------------------- | --------- | ---------- | ---------------------------- |
| BM_IsEmpty          | 0.86      | 1.16B/sec  | Simple container empty check |
| BM_BestAsk          | 1.16      | 870M/sec   | Map begin() - lowest ask     |
| BM_BestBid          | 3.18      | 315M/sec   | Map rbegin() - highest bid   |
| BM_GetOrdersAtPrice | 4.45      | 225M/sec   | Map find at specific price   |

**Analysis:** Query operations are sub-5ns, making them negligible overhead in the matching pipeline.

### Matching Operations (7 - 200 ns)

These represent actual order processing scenarios:

| Operation                         | Time (ns) | Throughput | Notes                           |
| --------------------------------- | --------- | ---------- | ------------------------------- |
| BM_ProcessOrder_NoMatch           | 7.52      | 133M/sec   | Order goes to book (no match)   |
| BM_ProcessOrder_PartialMatch      | 26.88     | 37M/sec    | Partial fill scenario           |
| BM_ProcessOrder_ExactMatch        | 81.36     | 12M/sec    | Full match - both orders filled |
| BM_ProcessOrder_WithRestingOrders | 185.61    | 5.4M/sec   | With 100 pre-populated orders   |
| BM_ProcessOrder_EmptyBook         | 200.22    | 5.0M/sec   | Empty book scenario             |

**Analysis:** Matching overhead varies based on scenario complexity. No-match is fastest (just book insertion), while exact match involves trade creation.

### Mutation Operations (> 500 ns)

These modify the order book state:

| Operation      | Time (ns) | Throughput | Notes                   |
| -------------- | --------- | ---------- | ----------------------- |
| BM_RemoveOrder | 577.07    | 1.7M/sec   | Remove + erase if empty |
| BM_AddOrder    | 853.94    | 1.17M/sec  | Map insert + deque push |

**Analysis:** These are the slowest operations as they involve actual data structure modifications.

---

## Suspected Variability Factors

Based on our investigation, the following factors affect benchmark results:

### 1. Price Distribution (Random vs Clustered)

| Scenario             | What Happens                                              |
| -------------------- | --------------------------------------------------------- |
| **Random prices**    | Most orders create new map keys → tests map operations    |
| **Clustered prices** | Orders append to existing deques → tests queue operations |

**Current benchmark uses random prices** from the OrderGenerator, which may not reflect real market conditions where multiple orders exist at the same price level.

### 2. Cache Effects

- **Hot data:** Map iterators are likely cached after first access
- **Cold start:** First iteration is typically slower
- **Subsequent iterations:** Benefit from CPU cache

### 3. Compiler Optimization

We used `benchmark::DoNotOptimize()` to prevent the compiler from optimizing away operations that return values but don't use them.

### 4. Test Data Specificity

- Current random price generation creates sparse price levels
- In production, order books typically have tighter price clustering
- This means **deque operations** are more common in real scenarios than our benchmarks show

---

## Methodology

### Benchmark Setup

- **Framework:** Google Benchmark v1.9.5
- **Build Type:** Release (optimizations enabled)
- **Iterations:** Automatic (Google Benchmark determines optimal count)
- **Warming:** Implicit via Google Benchmark's design

### Running Benchmarks

```bash
# Configure with benchmarking enabled
cmake -DENABLE_BENCHMARKING=ON -DBUILD_TESTS=ON ..

# Build
cmake --build . --target matching_benchmark

# Run and save results
./bin/matching_benchmark --benchmark_format=json --benchmark_out=benchmarks/results/<NAME>.json
cp benchmarks/results/<NAME>.json ../benchmarks/results/
```

### Interpreting Results

- **Time (ns):** Nanoseconds per operation (lower is better)
- **Throughput:** Operations per second (higher is better)
- **Iterations:** Number of times the operation ran to get reliable timing

---

## Future Improvements

### Known Gaps

1. ~~**Clustered price scenarios:** Need to add benchmarks with many orders at same price~~ ✅ **DONE**
2. **Latency percentiles (p50, p95, p99):** Not currently measured
3. **Multi-threaded benchmarks:** Not yet implemented
4. **Memory footprint:** Not measured

### Planned Benchmarks

- ~~Test with clustered price distributions~~ ✅ **COMPLETED**
- Add latency percentile tracking
- Measure memory allocation patterns
- Add concurrent access scenarios (future milestones)

---

## Clustered Price Benchmarks (2026-04-26)

### What We Changed

Added a new **clustering mode** to the OrderGenerator to support more realistic market scenarios:

```cpp
struct Config {
    // ... existing fields ...
    bool enable_clustering = false;
    uint64_t clustering_price = 100000;   // Base price for clustering
    uint32_t cluster_count = 100;          // Orders per cluster before switching
    uint32_t num_clusters = 3;             // Number of price levels
};
```

When enabled, the generator produces orders at the same price level for `cluster_count` orders, then switches to the next price level (useful for testing deque operations).

### Why We Did It

Our initial benchmarks used **random prices**, which primarily test map operations (creating new price levels). However, in real trading scenarios:

- Multiple orders typically exist at the **same price level**
- This triggers **deque operations** (push_back, pop_front) more frequently
- Random price benchmarks don't capture this realistic behavior

By adding clustered benchmarks, we can measure the performance impact of queue operations vs map operations.

### New Clustered Benchmark Results

| Benchmark                  | Time (ns) | Throughput | Description                           |
| -------------------------- | --------- | ---------- | ------------------------------------- |
| BM_AddOrder_Clustered      | 34.90     | 28.7M/sec  | 3 price levels, 1000 orders each      |
| BM_ProcessOrder_Clustered  | 57.23     | 17.5M/sec  | Book pre-filled with clustered orders |
| BM_AddOrder_HighClustering | 33.85     | 29.6M/sec  | Single price level, 10000 orders      |

### Comparison: Random vs Clustered

| Operation                     | Random Prices | Clustered Prices | Difference        |
| ----------------------------- | ------------- | ---------------- | ----------------- |
| BM_AddOrder                   | 853.94 ns     | 34.90 ns         | **24.5x faster!** |
| BM_ProcessOrder (with orders) | 185.61 ns     | 57.23 ns         | **3.2x faster**   |

### Analysis

The clustered benchmarks show significantly **faster performance** because:

1. **Cache locality:** Orders at same price level are stored together in the deque
2. **Fewer map operations:** No need to create new map entries for each order
3. **Sequential access:** Deque push_back is more cache-friendly than map insert

This confirms that our implementation is **well-optimized for realistic trading scenarios** where orders cluster at specific price levels.

---

## Extreme Integration Benchmark (2026-04-26)

### What We Changed

Added a new **extreme integration benchmark** that simulates a realistic market with:

- **10,000 pre-populated orders** in the order book
- **Clustered orders**: 9,000 orders at 3 main price levels (3,000 each at 100000, 100500, 101000)
- **Scattered orders**: 1,000 orders at random prices within range
- **Benchmark**: Process incoming orders against this loaded market

This represents a **realistic high-volume trading scenario** where the order book is already populated with liquidity.

### Why We Did It

Previous benchmarks tested operations in isolation:

- Individual add/remove operations
- Matching with small book sizes
- Clustered vs random price distributions

This extreme benchmark tests:

- **Performance under load** - How fast can we process orders when book is already large?
- **Realistic scenario** - A real exchange would have thousands of orders at various levels
- **Memory pressure** - How does the system handle large book sizes?

### Results

| Benchmark                  | Time (ns) | Throughput | Description               |
| -------------------------- | --------- | ---------- | ------------------------- |
| **BM_Integration_Extreme** | 154.08    | 6.5M/sec   | Extreme load (10K orders) |

### Scatter Fix Analysis (2026-04-26)

We initially tested with scattered orders at a wider price range ($10-$50), while clustered orders were at $10-$10.10. We hypothesized this might affect performance.

**Results after fixing scatter range to match clustered range ($10-$11):**

| Version | Time (ns) | Change |
|---------|-----------|--------|
| Original (scattered $10-$50) | 154.08 | - |
| Fixed (scattered $10-$11) | 153.44 | **-0.64 ns (0.4%)** |

**Conclusion:** The difference is negligible (~0.4%), indicating:
1. The scattered orders at higher prices were rarely matched anyway
2. The clustered price levels dominate performance
3. The system is already well-optimized for the critical path

This validates that our clustered benchmark represents the critical performance path.

### Analysis

The extreme integration benchmark shows that even with **10,000 pre-populated orders**, we can still process **~6.5 million orders per second**. This is significantly faster than the baseline (5M/sec with empty book) because:

1. **Better cache locality** - Orders are clustered at specific price levels
2. **Queue operations** - Most matching happens at the front of established queues
3. **No map overhead** - Most orders match at existing price levels, avoiding new map insertions

---

## Comparison History

| Date       | Notes                | Key Changes                                                      |
| ---------- | -------------------- | ---------------------------------------------------------------- |
| 2026-04-26 | Initial baseline     | First release                                                    |
| 2026-04-26 | Clustered benchmarks | Added clustering mode to OrderGenerator, tested deque operations |

---

## Lessons Learned

1. **Random vs Real Data:** Benchmark data significantly affects results - random prices test map operations, clustered prices test queue operations.

2. **Query Operations are Fast:** Sub-5ns for all query operations means minimal overhead.

3. **Matching is Efficient:** Even with trade creation, matching operates in 7-200ns range.

4. **Mutation is the Bottleneck:** Add/remove operations at ~500-850ns are the slowest but still reasonable for single-threaded operations.

---

## References

- **Design Document:** `../design_doc.md`
- **Source Code:** `../src/core/`
- **Benchmark Code:** `../benchmarks/matching_benchmark.cpp`

---

_Last updated: 2026-04-26 (Scatter Fix Analysis added)_
