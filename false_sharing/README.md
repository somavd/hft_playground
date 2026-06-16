# False Sharing Demonstration

This folder demonstrates false sharing in multi-threaded applications and how to prevent it using cache line alignment, a critical optimization for high-frequency trading systems.

## Files

- `alignas.cpp` - Demonstration of false sharing and its solution using `alignas`
- `README.md` - This file

## Compile and Run

```bash
cd false_sharing
g++ -O3 -std=c++20 -pthread alignas.cpp -o alignas
./alignas
```

## What is False Sharing?

False sharing occurs when multiple threads frequently modify different variables that happen to reside on the same cache line. Even though the threads are accessing different variables, the cache line becomes a point of contention because:

1. **Cache Lines**: CPUs load and store data in cache lines (typically 64 bytes)
2. **Cache Coherence**: When one core modifies a cache line, other cores must invalidate their copies
3. **Performance Impact**: This causes unnecessary cache invalidation and memory traffic

### Example Scenario
```cpp
struct FalseSharing {
    std::atomic<long long> counter1 {0};  // 8 bytes
    std::atomic<long long> counter2 {0};  // 8 bytes
    // Total: 16 bytes - fits in one 64-byte cache line!
};
```

When Thread 1 writes to `counter1` and Thread 2 writes to `counter2`, they contend for the same cache line even though they're accessing different variables.

## Code Analysis

### Problematic Structure (False Sharing)
```cpp
struct FalseSharing {
    std::atomic<long long> counter1 {0};
    std::atomic<long long> counter2 {0};
};
```

**Memory Layout:**
- `counter1`: 8 bytes (offset 0)
- `counter2`: 8 bytes (offset 8)
- **Total**: 16 bytes
- **Cache Line**: 64 bytes
- **Result**: Both variables in same cache line → False sharing

### Solution Structure (No False Sharing)
```cpp
struct alignas(64) PaddedAtomic {
    std::atomic<long long> value {0};
};

struct NoFalseSharing {
    PaddedAtomic counter1;  // 64 bytes aligned
    PaddedAtomic counter2;  // 64 bytes aligned
};
```

**Memory Layout:**
- `counter1`: 64 bytes (cache line 0)
- `counter2`: 64 bytes (cache line 1)
- **Total**: 128 bytes
- **Result**: Each variable in separate cache line → No false sharing

## How `alignas` Works

### `alignas` Specifier
```cpp
struct alignas(64) PaddedAtomic {
    std::atomic<long long> value {0};
};
```

**What it does:**
- Forces the compiler to align the structure to a 64-byte boundary
- Ensures each instance starts at the beginning of a cache line
- Prevents variables from sharing cache lines

**Why 64 bytes:**
- Typical cache line size on modern x86 processors
- Some architectures use 32 or 128 bytes
- Can be checked with `std::hardware_constructive_interference_size`

### Memory Layout Visualization

**With False Sharing:**
```
Cache Line 0 (64 bytes):
[ counter1 (8 bytes) ][ counter2 (8 bytes) ][ 48 bytes padding ]
Thread 1 writes counter1 → Invalidates Thread 2's cache line
Thread 2 writes counter2 → Invalidates Thread 1's cache line
```

**Without False Sharing:**
```
Cache Line 0 (64 bytes):
[ counter1 (8 bytes) ][ 56 bytes padding ]
Thread 1 writes counter1 → Only affects cache line 0

Cache Line 1 (64 bytes):
[ counter2 (8 bytes) ][ 56 bytes padding ]
Thread 2 writes counter2 → Only affects cache line 1
```

## Benchmark Results

### Expected Performance Difference
```
Running Benchmark...
False sharing time: XXX ms      // Slower due to cache contention
No false sharing time: XXX ms   // Faster due to separate cache lines
```

**Typical Results:**
- False sharing: 2-10x slower
- Performance gain depends on CPU architecture and core count
- More significant on systems with more cores

### Why the Difference?
1. **Cache Misses**: False sharing causes frequent cache misses
2. **Memory Bandwidth**: Unnecessary cache coherence traffic
3. **Core Contention**: Cores compete for the same cache line
4. **Pipeline Stalls**: Waiting for cache invalidation

## Code Walkthrough

### False Sharing Benchmark
```cpp
void runFalseSharing() {
    FalseSharing data;  // Both counters in same cache line
    
    std::thread t1([&data]() {
        for (int i = 0; i < NUM_ITERATIONS; ++i) {
            data.counter1.fetch_add(1, std::memory_order_relaxed);
        }
    });
    
    std::thread t2([&data]() {
        for (int i = 0; i < NUM_ITERATIONS; ++i) {
            data.counter2.fetch_add(1, std::memory_order_relaxed);
        }
    });
    
    t1.join();
    t2.join();
}
```

**What happens:**
- Thread 1 increments `counter1` → modifies cache line
- Thread 2 increments `counter2` → modifies same cache line
- Each write invalidates the other thread's cache
- Constant cache line bouncing between cores

### No False Sharing Benchmark
```cpp
void runNoFalseSharing() {
    NoFalseSharing data;  // Each counter in separate cache line
    
    std::thread t1([&data]() {
        for (int i = 0; i < NUM_ITERATIONS; ++i) {
            data.counter1.value.fetch_add(1, std::memory_order_relaxed);
        }
    });
    
    std::thread t2([&data]() {
        for (int i = 0; i < NUM_ITERATIONS; ++i) {
            data.counter2.value.fetch_add(1, std::memory_order_relaxed);
        }
    });
    
    t1.join();
    t2.join();
}
```

**What happens:**
- Thread 1 increments `counter1` → modifies cache line 0
- Thread 2 increments `counter2` → modifies cache line 1
- No cache line contention
- Each core works independently

## HFT Applications

### Market Data Processing
```cpp
struct alignas(64) MarketDataCounter {
    std::atomic<uint64_t> messages_processed {0};
    std::atomic<uint64_t> bytes_processed {0};
};
```

### Order Book Updates
```cpp
struct alignas(64) OrderBookStats {
    std::atomic<uint64_t> bid_updates {0};
    std::atomic<uint64_t> ask_updates {0};
};
```

### Risk Management
```cpp
struct alignas(64) PositionCounters {
    std::atomic<int64_t> long_positions {0};
    std::atomic<int64_t> short_positions {0};
};
```

## Best Practices

### 1. **Identify Contended Variables**
- Profile to find frequently accessed variables
- Look for variables accessed by different threads
- Check memory layout and cache line alignment

### 2. **Use `alignas` for Contended Variables**
```cpp
struct alignas(64) PaddedCounter {
    std::atomic<uint64_t> value {0};
    // Compiler adds padding to 64 bytes
};
```

### 3. **Consider Cache Line Size**
- Most x86: 64 bytes
- Some ARM: 64 bytes
- Some PowerPC: 128 bytes
- Use `std::hardware_constructive_interference_size` for portability

### 4. **Trade-offs**
- **Memory overhead**: Each padded variable uses full cache line
- **Cache pollution**: More memory usage
- **Performance gain**: Significant for contended variables

### 5. **Alternative Approaches**
```cpp
// Manual padding
struct ManualPadding {
    std::atomic<uint64_t> counter1 {0};
    char padding1[56];  // Manual padding to 64 bytes
    std::atomic<uint64_t> counter2 {0};
    char padding2[56];  // Manual padding to 64 bytes
};

// Using compiler-specific attributes
struct __attribute__((aligned(64))) CompilerPadded {
    std::atomic<uint64_t> value {0};
};
```

## Performance Characteristics

### Latency Impact
- **With false sharing**: 10-100ns per operation (cache miss penalty)
- **Without false sharing**: 1-5ns per operation (cache hit)

### Throughput Impact
- **With false sharing**: Limited by cache coherence traffic
- **Without false sharing**: Limited by memory bandwidth

### Scalability
- **With false sharing**: Degrades with more cores
- **Without false sharing**: Scales with core count

## Common Pitfalls

### 1. **Over-padding**
- Don't pad everything
- Only pad frequently contended variables
- Memory overhead can hurt performance

### 2. **Wrong Alignment Size**
- Use actual cache line size
- Don't assume 64 bytes on all platforms
- Check CPU documentation

### 3. **False Sharing in Arrays**
```cpp
// Problem: Array elements may share cache lines
std::atomic<int> counters[100];

// Solution: Use padded types
std::array<PaddedAtomic, 100> counters;
```

### 4. **False Sharing in Structs**
```cpp
// Problem: Struct fields may share cache lines
struct Stats {
    std::atomic<int> counter1;
    std::atomic<int> counter2;
};

// Solution: Align individual fields
struct AlignedStats {
    alignas(64) std::atomic<int> counter1;
    alignas(64) std::atomic<int> counter2;
};
```

## Related Concepts

- **Cache Coherence Protocols**: MESI, MOESI protocols
- **Cache Lines**: 64-byte memory blocks
- **Memory Hierarchy**: L1, L2, L3 caches
- **NUMA Architectures**: Non-uniform memory access
- **Cache Thrashing**: Excessive cache line bouncing
- **True Sharing**: Intentional sharing of data

## Detection and Debugging

### Performance Profiling
- Use `perf` to identify cache misses
- Look for high cache miss rates
- Check for cache coherence traffic

### Memory Layout Analysis
```cpp
#include <iostream>
#include <typeinfo>

struct Test {
    std::atomic<int> a;
    std::atomic<int> b;
};

int main() {
    std::cout << "Size: " << sizeof(Test) << std::endl;
    std::cout << "Alignment: " << alignof(Test) << std::endl;
    return 0;
}
```

### Hardware Counters
- Use hardware performance counters
- Monitor cache miss rates
- Track cache line invalidations

## Further Reading

- [False Sharing](https://en.wikipedia.org/wiki/False_sharing)
- [C++ alignas specifier](https://en.cppreference.com/w/cpp/language/alignas)
- [Cache Coherence](https://en.wikipedia.org/wiki/Cache_coherence)
- [HFT Performance Optimization](https://www.highfrequencytrading.com/)
- [Intel Optimization Manual](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
