# Benchmark Results

## Overview

This document tracks the performance baseline of the Matching Engine. All benchmarks measure individual operations to understand the performance characteristics of our order book implementation.

**Date:** 2026-04-26  
**Environment:** macOS, Apple Silicon M2 (10 cores @ 24 MHz)  
**Build:** Release mode with optimizations enabled

---

## Quick Summary Table

| Benchmark | Time (ns) | Throughput (ops/sec) | Category |
|-----------|-----------|---------------------|----------|
| BM_IsEmpty | 0.86 | 1.16B/sec | Query |
| BM_BestAsk | 1.16 | 870M/sec | Query |
| BM_BestBid | 3.18 | 315M/sec | Query |
| BM_GetOrdersAtPrice | 4.45 | 225M/sec | Query |
| BM_ProcessOrder_NoMatch | 7.52 | 133M/sec | Matching |
| BM_ProcessOrder_PartialMatch | 26.88 | 37M/sec | Matching |
| BM_ProcessOrder_ExactMatch | 81.36 | 12M/sec | Matching |
| BM_ProcessOrder_WithRestingOrders | 185.61 | 5.4M/sec | Matching |
| BM_ProcessOrder_EmptyBook | 200.22 | 5.0M/sec | Matching |
| BM_RemoveOrder | 577.07 | 1.7M/sec | Mutation |
| BM_AddOrder | 853.94 | 1.17M/sec | Mutation |

---

## Detailed Results

### Query Operations (< 5 ns)

These operations are extremely fast because they involve simple map lookups:

| Operation | Time (ns) | Throughput | Notes |
|-----------|-----------|-----------|-------|
| BM_IsEmpty | 0.86 | 1.16B/sec | Simple container empty check |
| BM_BestAsk | 1.16 | 870M/sec | Map begin() - lowest ask |
| BM_BestBid | 3.18 | 315M/sec | Map rbegin() - highest bid |
| BM_GetOrdersAtPrice | 4.45 | 225M/sec | Map find at specific price |

**Analysis:** Query operations are sub-5ns, making them negligible overhead in the matching pipeline.

### Matching Operations (7 - 200 ns)

These represent actual order processing scenarios:

| Operation | Time (ns) | Throughput | Notes |
|-----------|-----------|-----------|-------|
| BM_ProcessOrder_NoMatch | 7.52 | 133M/sec | Order goes to book (no match) |
| BM_ProcessOrder_PartialMatch | 26.88 | 37M/sec | Partial fill scenario |
| BM_ProcessOrder_ExactMatch | 81.36 | 12M/sec | Full match - both orders filled |
| BM_ProcessOrder_WithRestingOrders | 185.61 | 5.4M/sec | With 100 pre-populated orders |
| BM_ProcessOrder_EmptyBook | 200.22 | 5.0M/sec | Empty book scenario |

**Analysis:** Matching overhead varies based on scenario complexity. No-match is fastest (just book insertion), while exact match involves trade creation.

### Mutation Operations (> 500 ns)

These modify the order book state:

| Operation | Time (ns) | Throughput | Notes |
|-----------|-----------|-----------|-------|
| BM_RemoveOrder | 577.07 | 1.7M/sec | Remove + erase if empty |
| BM_AddOrder | 853.94 | 1.17M/sec | Map insert + deque push |

**Analysis:** These are the slowest operations as they involve actual data structure modifications.

---

## Suspected Variability Factors

Based on our investigation, the following factors affect benchmark results:

### 1. Price Distribution (Random vs Clustered)

| Scenario | What Happens |
|----------|--------------|
| **Random prices** | Most orders create new map keys → tests map operations |
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

1. **Latency percentiles (p50, p95, p99):** Not currently measured
2. **Clustered price scenarios:** Need to add benchmarks with many orders at same price
3. **Multi-threaded benchmarks:** Not yet implemented
4. **Memory footprint:** Not measured

### Planned Benchmarks

- Add latency percentile tracking
- Test with clustered price distributions
- Measure memory allocation patterns
- Add concurrent access scenarios (future milestones)

---

## Comparison History

| Date | Notes | Key Changes |
|------|-------|-------------|
| 2026-04-26 | Initial baseline | First release |

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

*Last updated: 2026-04-26*
