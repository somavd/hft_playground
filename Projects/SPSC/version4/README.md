# SPSC Queue with Prefetch + Bitmask (Version 4)

Lock-free SPSC queue with combined cache prefetching and power-of-2 bitmask optimizations. This is the best-performing version, achieving 19% improvement over the baseline.

## Files

- `spsc_prefetch_bitmask.cpp` - SPSC queue with prefetch + bitmask optimizations
- `README.md` - This file

## Compile and Run

```bash
cd Projects/SPSC/version6
g++ -O3 -std=c++20 -pthread spsc_prefetch_bitmask.cpp -o spsc_prefetch_bitmask
./spsc_prefetch_bitmask
```

## Optimizations

### 1. Power-of-2 Bitmask

```cpp
static constexpr size_t SIZE = 16384;  // Power of 2
size_t next_tail = (current_tail + 1) & (SIZE - 1);  // Bitmask instead of modulo
```

**Benefits:**
- Bitwise AND is 1 cycle vs division (20-90 cycles)
- Eliminates slow modulo operation
- Requires power-of-2 queue size

### 2. Cache Prefetching

```cpp
buffer_[current_tail] = item;
__builtin_prefetch(&buffer_[(current_tail + 2) & (SIZE - 1)], 0, 3);  // Prefetch next cache line
```

**Benefits:**
- Prefetches cache line 2 positions ahead
- Reduces cache misses by bringing data into cache before needed
- Works best with predictable access patterns (power-of-2 size)

### 3. Combined Effect

The combination of both optimizations works synergistically:
- Bitmask provides fast index calculation
- Prefetching hides memory latency
- Power-of-2 size enables both optimizations effectively

## Benchmark Results vs Version 3

| Run | Version 3 (Baseline) | Version 6 (Prefetch + Bitmask) | Difference |
|-----|---------------------|--------------------------------|------------|
| 1 | 27.4M msg/s | 32.0M msg/s | +17% |
| 2 | 27.6M msg/s | 31.7M msg/s | +15% |
| 3 | 26.6M msg/s | 33.0M msg/s | +24% |
| 4 | 26.6M msg/s | 31.5M msg/s | +18% |
| 5 | 26.9M msg/s | 34.2M msg/s | +27% |
| **Avg** | **27.0M msg/s** | **32.7M msg/s** | **+19% faster** |

## Performance Comparison Across All Versions

| Version | Optimization | Queue Size | Avg Throughput | vs v1 (Mutex) | vs v3 (Baseline) |
|---------|--------------|------------|----------------|---------------|------------------|
| v1 | Mutex | 10000 | ~6M msg/s | 1x | - |
| v2 | Atomic (no alignas) | 10000 | ~22M msg/s | 3.7x | - |
| v3 | Atomic + alignas | 10000 | 27.0M msg/s | 4.5x | baseline |
| **v4** | **+ prefetch + bitmask** | **16384** | **32.7M msg/s** | **5.5x** | **+19%** |

## Why Version 4 Works Best

1. **Power-of-2 size (16384):** Enables fast bitmask operation
2. **Bitmask optimization:** Replaces slow division with fast AND
3. **Prefetching:** Works effectively with predictable power-of-2 access pattern
4. **Synergy:** Fast index calculation + memory latency hiding = best performance

## Why Individual Optimizations Failed

During testing, individual optimizations were tried separately but failed:
- **Prefetching only (with non-power-of-2 size):** Failed because queue size 10000 (not power-of-2) made modulo slow, prefetch overhead outweighed benefits
- **Bitmask only (with static constexpr):** Failed because static constexpr changed memory layout, cache locality penalty outweighed instruction-level gains

The combination of both optimizations with power-of-2 size succeeded where each individually failed.

## Key Takeaways

1. **Optimizations work in combination:** Neither prefetching nor bitmask alone helped, but together they provide 19% improvement
2. **Power-of-2 size is critical:** Enables both bitmask and effective prefetching
3. **Memory layout matters:** Static constexpr vs runtime affects cache locality
4. **Benchmark everything:** Theoretical optimizations don't always translate to real-world gains

## Further Optimizations Possible

1. **Batch operations:** Push/pop multiple items with single atomic update
2. **NUMA awareness:** Pin threads and allocate memory on same NUMA node
3. **Custom allocator:** Cache-aligned memory allocation
4. **Relaxed memory ordering:** More aggressive use of relaxed ordering where safe

## Conclusion

Version 6 demonstrates that the right combination of optimizations can significantly improve performance. The key is understanding how optimizations interact and testing empirically rather than relying on theoretical gains alone.
