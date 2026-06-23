# SPSC Queue with Atomics + Cache Line Alignment (Version 3)

Lock-free Single Producer Single Consumer queue with atomic operations and cache line alignment to prevent false sharing. This version demonstrates the critical performance impact of false sharing in lock-free programming.

## Files

- `alignas_spsc_atomic.cpp` - Lock-free SPSC queue with `alignas(64)` on atomic variables
- `README.md` - This file

## Compile and Run

```bash
cd Projects/SPSC/version3
g++ -O3 -std=c++20 -pthread alignas_spsc_atomic.cpp -o alignas_spsc_atomic
./alignas_spsc_atomic
```

## Version Comparison

| Version | Synchronization | False Sharing Prevention | Avg Throughput | vs Mutex |
|---------|----------------|---------------------------|----------------|----------|
| **v1 (Mutex)** | `std::mutex` | N/A | ~6M msg/s | 1x (baseline) |
| **v2 (Atomic)** | `std::atomic` | None | ~22M msg/s | 3.8x faster |
| **v3 (Atomic + alignas)** | `std::atomic` | `alignas(64)` | ~47M msg/s | **8.1x faster** |

**Key Finding:** Adding `alignas(64)` provides a **2.1x speedup** over the basic atomic version (v2).

## Expected Results

```
Time taken: 215856667 ns
Throughput: 4.6327e+07 msg/s
```

**Actual Performance (5-run average):**
- Throughput: ~47 million messages/second
- Time: ~0.21 seconds for 10M messages
- **8.1x faster** than mutex-based version
- **2.1x faster** than atomic version without alignment

## What is False Sharing?

**False sharing** occurs when two independent variables share the same CPU cache line (typically 64 bytes). When one core writes to its variable, the entire cache line is invalidated, forcing the other core to reload it from memory.

**Example:**
```cpp
// Without alignment (bad)
struct Bad {
    std::atomic<size_t> head{0};  // Bytes 0-7
    std::atomic<size_t> tail{0};  // Bytes 8-15 — same cache line!
};
```

When producer writes to `tail`, consumer's cache line containing `head` is invalidated (~100ns penalty per bounce).

## The Solution: alignas(64)

```cpp
// With alignment (good)
struct Good {
    alignas(64) std::atomic<size_t> head{0};  // Bytes 0-63 (own cache line)
    alignas(64) std::atomic<size_t> tail{0};  // Bytes 64-127 (own cache line)
};
```

Each atomic variable gets its own cache line, eliminating false sharing.

## Code Analysis

### SPSCQueue Class

```cpp
template<typename T>
class SPSCQueue {
private:
    std::vector<T> buffer_;
    alignas(64) std::atomic<size_t> head{0};  // Own cache line
    alignas(64) std::atomic<size_t> tail{0};  // Own cache line
    size_t max_size_;

public:
    SPSCQueue(size_t max_size = 10000) : max_size_(max_size) {
        buffer_.resize(max_size);
    }

    bool push(const T& item) {
        size_t current_tail = tail.load(std::memory_order_relaxed);
        size_t next_tail = (current_tail + 1) % max_size_;
        if (next_tail == head.load(std::memory_order_acquire)) {
            return false; // Queue is full
        }
        buffer_[current_tail] = item;
        tail.store(next_tail, std::memory_order_release);
        return true;
    }

    bool pop(T& item) {
        size_t current_head = head.load(std::memory_order_relaxed);
        if(current_head == tail.load(std::memory_order_acquire)) {
            return false; // Queue is empty
        }
        item = buffer_[current_head];
        head.store((current_head + 1) % max_size_, std::memory_order_release);
        return true;
    }
};
```

**Key Difference from Version 2:**
- Lines 23-24: `alignas(64)` ensures each atomic has its own cache line
- Everything else is identical to version 2

## Performance Impact

### Without alignas(64) (Version 2)

**Memory Layout:**
```
Cache Line 0: [head (4 bytes)] [tail (4 bytes)] [padding (56 bytes)]
```

**Execution:**
1. Producer writes to `tail` → invalidates cache line
2. Consumer's `head` is on same line → cache miss
3. Consumer reloads from memory → ~100ns penalty
4. This happens on every push/pop → massive overhead

### With alignas(64) (Version 3)

**Memory Layout:**
```
Cache Line 0: [head (4 bytes)] [padding (60 bytes)]
Cache Line 1: [tail (4 bytes)] [padding (60 bytes)]
```

**Execution:**
1. Producer writes to `tail` → invalidates only cache line 1
2. Consumer's `head` is on cache line 0 → still valid
3. No cache miss → no penalty
4. Each core operates independently → maximum throughput

## Benchmark Results (5 runs)

| Run | v1 (Mutex) | v2 (Atomic) | v3 (Atomic + alignas) |
|-----|------------|-------------|----------------------|
| 1 | 6.31M msg/s | 23.5M msg/s | 46.3M msg/s |
| 2 | 6.30M msg/s | 21.4M msg/s | 49.4M msg/s |
| 3 | 5.35M msg/s | 22.7M msg/s | 49.3M msg/s |
| 4 | 6.15M msg/s | 21.3M msg/s | 45.0M msg/s |
| 5 | 5.41M msg/s | 22.4M msg/s | 46.1M msg/s |
| **Avg** | **5.85M msg/s** | **22.3M msg/s** | **47.2M msg/s** |

## Why This Matters for HFT

In high-frequency trading:
- **Latency is critical** — Every nanosecond counts
- **False sharing is invisible** — Code looks correct but performs poorly
- **Cache coherence is expensive** — Cross-core communication costs
- **SPSC is common** — Feed thread → strategy thread pattern

**Real-world impact:**
- Without alignment: 22M msg/s → 45ns per message
- With alignment: 47M msg/s → 21ns per message
- **2x latency reduction** from one keyword!

## When to Use alignas(64)

**Use when:**
- Multiple threads write to different variables frequently
- Variables are in the same struct/class
- Performance is critical (HFT, gaming, real-time systems)
- Using lock-free data structures

**Don't overuse:**
- Increases memory usage (padding wastes space)
- Only needed for frequently-written shared variables
- Not needed for read-only or single-threaded code

## Further Optimizations

This version still has room for improvement:

1. **Power-of-2 bitmask** — Replace `% max_size_` with `& (SIZE-1)` (division is slow)
2. **Compile-time size** — Use template parameter instead of runtime
3. **C-array buffer** — Remove std::vector indirection
4. **size_t indices** — Use unsigned 64-bit instead of signed 32-bit

These optimizations could push throughput even higher (potentially 50-100M msg/s).

## Key Takeaways

1. **False sharing is a silent performance killer** — Code looks correct but runs slowly
2. **alignas(64) is a simple fix** — One keyword, 2x speedup
3. **Cache line alignment is critical for lock-free** — Atomics without alignment suffer
4. **Benchmark before optimizing** — Measure to identify the real bottleneck
5. **HFT requires attention to hardware** — Understanding CPU architecture matters

## Related Concepts

- **False Sharing** — Cache line contention between cores
- **Cache Coherence** — How CPUs maintain consistent memory views
- **Cache Line Size** — Typically 64 bytes on modern CPUs
- **Memory Alignment** — Ensuring data starts at specific boundaries
- **Lock-Free Programming** — Synchronization without mutexes
