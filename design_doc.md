# Matching Engine Design Document

## Project Goals
- **Milestone 1**: Single symbol, LIMIT orders, single-threaded matching (target: working + baseline measurement, likely 100K-500K/sec)
- **Milestone 2**: Performance optimization (target: 1-2M orders/sec single-threaded)
- **Milestone 3**: Concurrency, multi-symbol sharding (target: 10M+ orders/sec aggregate)
- **Milestone 4**: Full feature set (MARKET, IOC, FOK, Cancel/Modify, Gateway, Publishers)

### Why These Targets?
- **200ns per order** (5M/sec) is ~600 CPU cycles - basically impossible with tree lookups + matching logic
- **500ns-1μs per order** (1-2M/sec) is achievable with memory pools, cache-friendly structures, zero allocations
- **10M+ aggregate** requires parallelism - sharding by symbol across cores
- Reference: LMAX Disruptor achieved ~6M messages/sec for simple message passing (not full matching)

## Tech Stack
- **Language**: C++
- **Build**: CMake
- **Testing**: Google Test
- **Benchmarking**: Google Benchmark
- **Platform**: macOS (primary), Linux (secondary)

---

# Milestone 1: Foundation

## Components to Build
1. Order Gateway: Receives incoming orders, validates, timestamps
2. Order Book: Maintains bid/ask price levels
3. Matching Engine: Core matching logic (price-time priority)
4. Trade Publisher: Outputs matched trades
5. Market Data Publisher: Publishes order book updates

---

## 1. Data Structures

### 1.1 Order Struct

**YOUR TASK**: Think through each field. What do you need? What types? Why?

| Field | Type | Your Reasoning | Memory Size (bytes) |
|-------|------|----------------|-------------|
| order_id | uint64_t | there can be billions of orders coming in, therefore, we have to prepare accordingly| 8 |
| timestamp | uint64_t | let's stick with UNIX timestamp (in nanoseconds) | 8 |
| price | uint64_t | price with 0.0001 increments, therefore 23.34 --> 233400 | 8 |  
| symbol_id | uin32_t | We store a map of <uint32_t, String> --> once, but then the hot path gets unblocked and no string comparisons takes place | 4 |
| quantity | uint32_t | I believe even this is pretty big! but it should do that job | 4 |
| remaining_quantity | uin32_t | Easier to track the original quantity and remaining quantity in separate variables| 4 |
| side | Enum | Enum type always makes sense here | 1 |
| order_type| Enum | enum class OrderType : uint8_t { LIMIT, MARKET, IOC, FOK }| 1 |
| status | Enum | enum class Status : uint8_t { NEW, PARTIALLY_FILLED, FILLLED, CANCELLED }| 1 |

**Memory Layout Questions to Consider**:
- What's the total size of your struct? 40 Bytes
- Is it a power of 2? Does that matter? No! there is automatic padding applied
- What's a cache line size? How does your struct fit? --> The structure is well within 64 Bytes, so it will fit the cache line
- Should fields be ordered differently for alignment? --> Yes! that is how it is done!

---

### 1.2 Trade Struct

**YOUR TASK**: What gets produced when two orders match?

| Field | Type | Your Reasoning | Memory Size (bytes)
|-------|------|----------------|---------------------|
| trade_id | uint64_t | Accounting for large number of trades| 8 |
| buy_order_id | uint64_t | associated buy order | 8 |
| sell_order_id| uint64_t | associated sell order | 8 |
| price | uint64_t | price at which the trade occurred | 8 |
| timestamp | uint64_t | UNIX timestamp in nanoseconds | 8 |
| quantity | uint32_t | quantity traded | 4 |
| aggressor | enum | uint8_t { BUY, SELL } | 1 |


---

### 1.3 Order Book Structure

**YOUR TASK**: This is the critical data structure. Think about:

Q) How do you organize orders by price level?
  - std::map keeps prices sorted (O(log n) insert/lookup/delete)
  - Price is the key (your fixed-point uint64_t)
  - No need for segment trees or fancy structures for Milestone 1


Q) How do you maintain time priority within a price level?
  - Just append new orders to the back of the std::deque. No sorting needed.
  - Orders arrive in time order naturally
  - deque.push_back(order) → maintains FIFO
  - When matching, take from deque.front() (oldest first)
  - When order is filled, deque.pop_front()

Q) How do you quickly find the best bid? Best ask?

| Side | Best Price | How to Find |
|------|------------|-------------|
| Best Bid | Highest BUY price | bids.rbegin() (last element, highest key) |
| Best Ask | Lowest SELL price | asks.begin() (first element, lowest key) |

Both are O(1) operations on std::map iterators.

Q)What operations need to be fast? (insert, cancel, match, query best)

| Operation | Frequency | Complexity with std::map |
|-----------|-----------|---------------------------|
| Find best bid/ask | Every match attempt | O(1) via iterator |
| Remove front order (match) | Every trade | O(1) deque pop_front |
| Insert new order | Every new order | O(log n) map + O(1) deque |
| Cancel order | Less frequent | O(log n) + O(n) search in deque* |

*Cancel is the weak spot - finding a specific order in a deque is O(n). Milestone 2 optimization could add a hashmap<order_id, iterator> for O(1) cancel.

Q) Choice of Data Structure

**Choice**: `std::map<price, std::deque<Order>>`

**Why**: 

| Aspect | std::deque | std::list |
|--------|--------------|-------------|
| Memory layout | Chunked contiguous | Scattered (linked nodes) |
| Cache performance | Better | Worse (pointer chasing) |
| Front/back insert/remove | O(1) | O(1) |
| Random access | O(1) | O(n) |

`std::deque` - better cache locality, same O(1) front/back operations.

**Final Data Structure Summary**

```cpp
OrderBook {
    std::map<uint64_t, std::deque<Order>> bids;  // price → orders (high to low)
    std::map<uint64_t, std::deque<Order>> asks;  // price → orders (low to high)
}
```

**Operations**:
- best_bid() → bids.rbegin()
- best_ask() → asks.begin()
- add_order(order) → map[price].push_back(order)
- match() → pop from front of best price level's deque

---

## 2. Order Gateway

### 2.1 Data Source (Milestone 1)
**Decision**: In-memory order generator (simulated)
**Rationale**: Focus on core matching logic first. Socket-based gateway comes in Milestone 4.

### 2.2 Validation Checks

**YOUR TASK**: List what you'll validate and why:


| Check | Why It Matters | Will `uint` handle it? |
|-------|----------------|------------------------------|
| Price > 0 | Zero price is invalid for LIMIT orders | No - need explicit check |
| Quantity > 0 | Can't trade zero shares | No - need explicit check |
| Valid symbol_id | Symbol must exist in your symbol map | No - need lookup |
| Valid side | Must be BUY or SELL | Enum helps, but validate input |
| Valid order_type | Must be known type (LIMIT for M1) | Enum helps |
| Timestamp not in future | Clock skew / bad data | Need explicit check |
| Timestamp not too stale | Reject orders older than X seconds | Need explicit check |
| Price within bands | Prevent fat-finger errors (e.g., 100x market price) | Advanced - Milestone 4 |
| Quantity within limits | Max order size rules | Advanced - Milestone 4 |

---

## 3. Matching Engine Logic

### 3.1 Matching Algorithm

**Algorithm**: Price-time priority

```
Price-Time Priority - The Rules
1. PRICE PRIORITY:    Better price always wins
                      - For BUY: higher price = better
                      - For SELL: lower price = better
2. TIME PRIORITY:     At same price, earlier order wins
                      - First in, first out (FIFO)
3. EXECUTION PRICE:   Trade happens at the RESTING order's price
                      (the order that was already in the book)
```

**Pseudo-code for the matching algorithm**

```txt
When a BUY order arrives:
  1. WHILE (incoming.remaining_quantity > 0):
       a. Find best_ask (lowest sell price)
       b. IF no asks exist OR incoming.price < best_ask.price:
            → STOP matching (no compatible sellers)
       c. Get oldest order at best_ask price (front of deque)
       d. Calculate trade_quantity = MIN(incoming.remaining, resting.remaining)
       e. CREATE TRADE(price=resting.price, quantity=trade_quantity)
       f. Update both orders' remaining_quantity
       g. IF resting order fully filled → remove from book
       h. IF price level empty → remove price level from map
  
  2. IF incoming.remaining_quantity > 0:
       → Add to bids map at incoming.price
When a SELL order arrives:
  (mirror logic - match against bids, leftover goes to asks)
```

**Edge Cases to Consider**:
| Edge Case | What Happens? |
|-----------|---------------|
| Incoming qty > resting qty |  1) Fully consume the resting order (trade for resting.qty) 2) Remove resting order from book 3) Loop continues - try to match next resting order 4) If no more matches, remaining incoming qty rests in book |
| Incoming matches multiple orders |  The WHILE loop handles this naturally: consume first order → remove it → find next best → consume → repeat until incoming is filled or no more matches |
| No match exists | Order goes directly to bids (if BUY) or asks (if SELL) |

---

## 4. Test Case Design

### 4.1 Basic Matching Tests

| Test Name | Setup | Action | Expected Outcome |
|-----------|-------|--------|------------------|
| exact_match_buy | ASK: 100 shares @ $10.00 | BUY 100 @ $10.00 | Trade: 100 @ $10.00. Book: empty |
| exact_match_sell | BID: 100 shares @ $10.00 | SELL 100 @ $10.00 | Trade: 100 @ $10.00. Book: empty |
| partial_fill_incoming | ASK: 50 shares @ $10.00 | BUY 100 @ $10.00 | Trade: 50 @ $10.00. Book: BID 50 @ $10.00 |
| partial_fill_resting | ASK: 100 shares @ $10.00 | BUY 50 @ $10.00 | Trade: 50 @ $10.00. Book: ASK 50 @ $10.00 |
| no_match_buy_too_low | ASK: 100 @ $10.00 | BUY 100 @ $9.00 | No trade. Book: ASK 100 @ $10.00, BID 100 @ $9.00 |
| no_match_sell_too_high | BID: 100 @ $10.00 | SELL 100 @ $11.00 | No trade. Book: BID 100 @ $10.00, ASK 100 @ $11.00 |
| time_priority | ASK: Order A: 50 @ $10 (T1), Order B: 50 @ $10 (T2) | BUY 50 @ $10.00 | Trade: 50 @ $10.00 with Order A. Book: ASK 50 @ $10.00 (Order B) |
| multi_fill | ASK: 50 @ $10.00, 50 @ $10.00, 50 @ $10.05 | BUY 120 @ $10.05 | Trades: 50 @ $10.00, 50 @ $10.00, 20 @ $10.05. Book: ASK 30 @ $10.05 |
| Empty book | BUY into empty book | Should just add to bids, no crash |
| Price level cleanup | Match that empties a price level | Verify level is removed from map |
| Multiple price levels | BUY that sweeps through 3 price levels | Tests the loop correctly |
| Status updates | Check order status after partial fill | Verify PARTIALLY_FILLED is set |

### 4.2 Order Book State Tests

| Test Name | Description |
|-----------|-------------|
| add_order_creates_level | Adding order to empty book creates new price level |
| add_order_existing_level | Adding order at existing price appends to deque |
| best_bid_correct | After multiple inserts, best_bid returns highest price |
| best_ask_correct | After multiple inserts, best_ask returns lowest price |
| remove_order_cleans_level | Removing last order at price level removes the level |

---

## 5. Spring Planning

Sprint Overview
| Task | Description | Estimated Effort | Dependencies |
|------|-------------|------------------|--------------|
| S1.1 | Project setup (CMake, folder structure, GTest, GBench) | Small | None |
| S1.2 | Implement data structures (Order, Trade, Enums) | Small | S1.1 |
| S1.3 | Implement OrderBook (add, best_bid/ask, remove) | Medium | S1.2 |
| S1.4 | Implement MatchingEngine (price-time priority algo) | Medium | S1.3 |
| S1.5 | Implement basic validation (OrderValidator) | Small | S1.2 |
| S1.6 | Write unit tests (matching + order book) | Medium | S1.4 |
| S1.7 | Implement simple order generator (for testing) | Small | S1.2 |
| S1.8 | Basic benchmark harness | Small | S1.4 |
| S1.9 | Integration test (end-to-end flow) | Small | All |
