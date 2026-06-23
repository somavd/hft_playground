# SPSC Queue with Mutex (Version 1)

This folder contains a Single Producer Single Consumer (SPSC) queue implementation using `std::mutex` for synchronization. This is a baseline implementation to compare against lock-free implementations.

## Files

- `spsc_mutex.cpp` - SPSC queue implementation with mutex-based synchronization
- `README.md` - This file

## Compile and Run

```bash
cd Projects/SPSC/version1
g++ -O3 -std=c++20 -pthread spsc_mutex.cpp -o spsc_mutex
./spsc_mutex
```

## Expected Results

```
Time taken: 1668940708 ns
Throughput: 5.99182e+06 msg/s
```

**Actual Performance:**
- Throughput: ~6 million messages/second
- Time: ~1.67 seconds for 10M messages
- Limited by mutex contention and context switching

## What is SPSC?

SPSC stands for **Single Producer Single Consumer** - a concurrency pattern where:
- **One thread** only produces (pushes) data
- **One thread** only consumes (pops) data
- No other threads access the queue

**Why SPSC is Special:**
- Simpler than MPMC (Multi-Producer Multi-Consumer)
- Enables lock-free optimizations
- Common in HFT systems (one feed thread, one processing thread)
- Predictable performance characteristics

## Code Analysis

### SPSCQueue Class

```cpp
template<typename T>
class SPSCQueue {
private:
    std::vector<T> buffer_;  // Pre-allocated circular buffer
    size_t head = 0;
    size_t tail = 0;
    std::mutex mutex_;
    size_t max_size_;

public:
    SPSCQueue(size_t max_size = 10000) : max_size_(max_size) {
        buffer_.resize(max_size);
    }

    bool push(const T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t next_tail = (tail + 1) % max_size_;
        if (next_tail == head) {
            return false; // Queue is full
        }
        buffer_[tail] = item;
        tail = next_tail;
        return true;
    }

    bool pop(T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (head == tail) {
            return false; // Queue is empty
        }
        item = buffer_[head];
        head = (head + 1) % max_size_;
        return true;
    }
};
```

**Key Components:**
- **std::vector**: Pre-allocated circular buffer for deterministic memory
- **head/tail indices**: Track read/write positions in circular buffer
- **std::mutex**: Mutual exclusion for thread safety
- **max_size_**: Maximum queue size (10,000 by default)
- **push()**: Thread-safe insertion with full check, returns bool
- **pop()**: Thread-safe removal with empty check, returns bool

**Design Decisions:**
- Uses circular buffer (not std::queue) for better cache locality
- Mutex protects entire critical section (index checks + buffer access)
- Returns `bool` from both `push()` and `pop()` for status
- Pre-allocated buffer prevents dynamic allocation overhead
- Producer implements busy-wait backpressure when queue is full
- No condition variable (uses busy-wait in consumer)

### Message Structure

```cpp
struct Message {
    uint64_t sequence;
};
```

**Purpose:**
- Sequence number for ordering verification
- Asserts ensure no messages are lost or reordered
- Can be extended with payload data

### Timer Class

```cpp
class Timer {
public:
    static uint64_t now() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count();
    }
};
```

**Purpose:**
- High-resolution timing in nanoseconds
- Uses `std::chrono::high_resolution_clock`
- Static method for convenient access

### Producer Function

```cpp
void Producer(SPSCQueue<Message>& queue) {
    Message msg;
    for(size_t i=0; i<ITERATIONS; i++) {
        msg.sequence = i;
        while(!queue.push(msg)) {
            // Backpressure: busy-wait if queue is full
        }
    }
}
```

**What it does:**
1. Creates message with sequence number
2. Attempts to push message to queue
3. Busy-waits if queue is full (push returns false)
4. Retries until push succeeds
5. Pushes 10 million messages to queue
6. Returns when complete

**Characteristics:**
- Single producer (by design)
- No synchronization needed for message creation
- Only locks during push operation
- Implements busy-wait backpressure when queue is full
- Prevents unbounded memory growth
- Uses spin-wait for lowest latency (no sleep overhead)

### Consumer Function

```cpp
void Consumer(SPSCQueue<Message>& queue) {
    Message msg;
    for(size_t i=0; i<ITERATIONS; i++) {
        while(!queue.pop(msg)) {
            // Busy wait
        }
        assert(msg.sequence == i);
    }
}
```

**What it does:**
1. Tries to pop from queue
2. Busy-waits if queue is empty
3. Verifies sequence number (no reordering)

**Characteristics:**
- Single consumer (by design)
- Busy-wait on empty queue (spins CPU)
- Asserts ensure correct ordering
- Only locks during pop operation

### Main Function

```cpp
int main() {
    SPSCQueue<Message> queue(10000);
    auto start = Timer::now();
    std::thread producer_thread(Producer, std::ref(queue));
    std::thread consumer_thread(Consumer, std::ref(queue));
    
    producer_thread.join();
    consumer_thread.join();

    auto end = Timer::now();
    double seconds = (end - start) / 1e9;
    double throughput = ITERATIONS / seconds;
    
    std::cout << "Time taken: " << end - start << " ns" << std::endl;
    std::cout << "Throughput: " << throughput << " msg/s" << std::endl;
    return 0;
}
```

**What it does:**
1. Creates queue with max size 10,000
2. Records start time
3. Spawns producer and consumer threads
4. Waits for both to complete
5. Records end time
6. Calculates and prints performance metrics

**Thread Safety:**
- Queue is passed by reference with `std::ref`
- Both threads access same queue safely via mutex
- Timing is done in main thread (no data races)

## Performance Characteristics

### Mutex Overhead

**Lock Contention:**
- Each push/pop acquires mutex
- Context switch if thread is preempted
- Cache line bouncing between cores
- Kernel involvement for contended locks

**Typical Costs:**
- Uncontended lock: ~10-50 nanoseconds
- Contended lock: ~100-1000 nanoseconds
- Context switch: ~1-10 microseconds

### Busy-Wait vs Condition Variable

**Current Implementation (Busy-Wait):**
```cpp
while(!queue.pop(msg)) {
    // Busy wait - spins CPU
}
```

**Pros:**
- Lowest latency when data is available
- No context switch overhead
- Simple implementation

**Cons:**
- Wastes CPU cycles when queue is empty
- High power consumption
- Can cause priority inversion

**Alternative (Condition Variable):**
```cpp
std::unique_lock<std::mutex> lock(mutex_);
cv_.wait(lock, [this] { return !queue_.empty(); });
```

**Pros:**
- No CPU waste when waiting
- Lower power consumption
- Better for variable production rates

**Cons:**
- Higher latency (context switch)
- More complex implementation
- Kernel involvement

### Throughput vs Latency

**This Implementation:**
- Optimized for throughput (10M messages)
- Busy-wait minimizes latency
- Mutex limits maximum throughput

**Trade-offs:**
- High throughput: ~1-10M msg/s
- Moderate latency: ~100-1000 ns
- CPU usage: 100% when waiting

## HFT Applications

### Market Data Feed

**Use Case:**
```cpp
// Producer: Market data feed thread
void MarketDataProducer(SPSCQueue<MarketData>& queue) {
    while(running) {
        MarketData data = exchange.receive();
        queue.push(data);
    }
}

// Consumer: Trading strategy thread
void StrategyConsumer(SPSCQueue<MarketData>& queue) {
    while(running) {
        MarketData data;
        while(!queue.pop(data)) {}  // Busy-wait for lowest latency
        execute_strategy(data);
    }
}
```

**Why SPSC:**
- One feed connection (single producer)
- One strategy instance (single consumer)
- Lowest latency is critical
- Predictable message ordering

### Order Routing

**Use Case:**
```cpp
// Producer: Order generation
void OrderProducer(SPSCQueue<Order>& queue) {
    while(running) {
        Order order = generate_order();
        queue.push(order);
    }
}

// Consumer: Order routing
void OrderRouter(SPSCQueue<Order>& queue) {
    while(running) {
        Order order;
        while(!queue.pop(order)) {}
        route_to_exchange(order);
    }
}
```

**Why SPSC:**
- One order generation source
- One routing destination
- Minimal latency required
- No need for multiple consumers

## Design Decisions

### Why std::queue?

**Pros:**
- Simple, well-tested
- Standard library implementation
- Easy to understand

**Cons:**
- Not optimized for SPSC
- Dynamic allocation overhead
- Cache-unfriendly for large queues

**Alternative:** Custom circular buffer (used in lock-free version)

### Why Mutex?

**Pros:**
- Simple to implement correctly
- Works for any data type
- Standard library support

**Cons:**
- Performance overhead
- Can cause priority inversion
- Limits scalability

**Alternative:** Lock-free with atomics (see version 2)

### Why Busy-Wait?

**Pros:**
- Lowest latency
- Simple implementation
- No kernel involvement

**Cons:**
- Wastes CPU cycles
- High power consumption
- Not suitable for all systems

**Alternative:** Condition variable for variable production rates

### Why No Condition Variable?

**Reasoning:**
- HFT systems prioritize latency over CPU efficiency
- Producer is continuously pushing (queue rarely empty)
- Busy-wait provides more predictable latency
- Simpler implementation for baseline comparison

## Known Issues

### 1. Busy-Wait CPU Waste

**Issue:** Consumer spins at 100% CPU when queue is empty.

**Fix:** Use condition variable or exponential backoff:
```cpp
int backoff = 1;
while(!queue.pop(msg)) {
    std::this_thread::sleep_for(std::chrono::nanoseconds(backoff));
    backoff = std::min(backoff * 2, 1000);
}
```

### 3. std::queue Dynamic Allocation

**Issue:** Each push may allocate memory, causing unpredictable latency.

**Fix:** Use pre-allocated circular buffer (see lock-free version).

## Performance Optimization Tips

### 1. Use Lock-Free for SPSC

Since SPSC has known producer/consumer pattern, lock-free is possible:
- Use atomic head/tail indices
- No mutex overhead
- Better cache locality
- Higher throughput

### 2. Pre-allocate Memory

Avoid dynamic allocations in hot path:
- Use circular buffer
- Reserve capacity upfront
- Use memory pools

### 3. Optimize Cache Usage

- Keep queue in cache-friendly layout
- Avoid false sharing
- Use cache line alignment

### 4. Consider NUMA

For multi-socket systems:
- Pin threads to same NUMA node
- Allocate memory on correct node
- Minimize cross-node traffic

## Comparison with Other Approaches

### vs Lock-Free SPSC

**Mutex (This Implementation):**
- Simpler to implement
- Works with any data type
- Lower throughput
- Higher latency

**Lock-Free:**
- More complex
- Requires careful memory ordering
- Higher throughput
- Lower latency
- No context switches

### vs MPMC Queue

**SPSC:**
- Simpler synchronization
- Better performance
- Limited to single producer/consumer

**MPMC:**
- More complex synchronization
- Lower performance
- Supports multiple producers/consumers

## Future Improvements

1. **Lock-Free Implementation:** Use atomics for zero-wait synchronization
2. **Circular Buffer:** Pre-allocated memory for deterministic latency
3. **Batch Operations:** Push/pop multiple items at once
4. **Backpressure:** Bounded queue with producer throttling
5. **Latency Distribution:** Measure individual message latencies
6. **NUMA Awareness:** Optimize for multi-socket systems

## Related Concepts

- **SPSC vs MPMC:** Single vs multiple producer/consumer patterns
- **Lock-Free Programming:** Synchronization without mutexes
- **Memory Ordering:** Atomic operations and visibility
- **Cache Coherence:** CPU cache synchronization
- **Context Switching:** OS thread scheduling overhead
- **Busy-Waiting:** Spinning vs blocking synchronization

## Further Reading

- [C++ Concurrency in Action](https://www.manning.com/books/c-plus-plus-concurrency-in-action-second-edition)
- [Lock-Free Programming](https://preshing.com/20120612/an-introduction-to-lock-free-programming/)
- [HFT Performance Optimization](https://www.highfrequencytrading.com/)
- [C++ Atomics](https://en.cppreference.com/w/cpp/atomic)
- [Memory Models](https://preshing.com/20130922/acquire-and-release-semantics/)
