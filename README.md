# HFT Playground

A collection of C++ benchmarks focused on high-frequency trading concepts, with each concept demonstrated in its own separate directory.

## 📁 Project Structure

Each folder contains a self-contained benchmark demonstrating a specific concept:

- **`atomic/`** - Comparison of plain int, atomic, and mutex operations
- **`fetch_add/`** - Atomic fetch_add with relaxed memory ordering
- **`release_and_acquire/`** - Memory order release/acquire semantics

## 🛠️ Building and Running

Each benchmark can be built and run independently:

### Atomic Operations Benchmark
```bash
cd atomic
g++ -O3 -std=c++20 -pthread src.cpp -o benchmark
./benchmark
```

### Fetch Add Benchmark
```bash
cd fetch_add
g++ -O3 -std=c++20 -pthread src.cpp -o benchmark
./benchmark
```

### Memory Order Benchmark
```bash
cd release_and_acquire
g++ -O3 -std=c++20 -pthread src.cpp -o benchmark
./benchmark
```

## 📊 Performance Analysis

For detailed performance analysis, use perf:

```bash
# In any benchmark directory
perf stat ./benchmark
```

## 📚 Concepts Covered

- **Atomic Operations**: Thread-safe operations without locks
- **Memory Ordering**: Relaxed, acquire, release semantics
- **Synchronization**: Mutex vs atomic performance comparison
- **Data Races**: Understanding undefined behavior in concurrent code

Each directory contains focused examples demonstrating these concepts in isolation.
