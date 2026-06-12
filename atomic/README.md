# Atomic vs Normal Counter Comparison

This benchmark compares the performance and correctness between plain integer operations and atomic operations in a multi-threaded environment.

## Compile

```bash
g++ -O3 -std=c++20 -pthread src.cpp -o benchmark
```

## Run

```bash
./benchmark
```

## What it Tests

- **Plain int**: Demonstrates data race behavior (undefined results)
- **Atomic**: Shows thread-safe operations with guaranteed correctness

## Key Insights

- Plain int operations are faster but have undefined behavior due to data races
- Atomic operations provide thread safety with performance overhead
- Results may vary depending on CPU architecture and compiler optimizations
