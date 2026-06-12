# Performance Comparison Suite

This directory contains comprehensive performance comparisons between different synchronization methods and memory models used in high-frequency trading applications.

## 📁 Structure

```
performance_comparison/
├── performance_comparison.cpp  # Main comparison suite with all tests
├── README.md                   # This file
└── performance_comparison    # Compiled benchmark executable
```

## 🚀 Running the Main Comparison Suite

```bash
cd performance_comparison
g++ -O3 -std=c++20 -pthread performance_comparison.cpp -o performance_comparison
./performance_comparison
```

## 📊 Comparison Categories

### 1. Normal vs Atomic Counter
Compares performance and correctness between:
- Plain integer operations (data race demonstration)
- Atomic integer operations

**Key Metrics:**
- Execution time comparison
- Result correctness verification
- Thread safety validation

### 2. Fetch Add Memory Models
Compares different memory ordering semantics:
- **Relaxed**: No ordering guarantees, fastest
- **Acquire-Release**: Synchronization without full sequential consistency
- **Sequentially Consistent**: Default, strongest guarantees

**Key Metrics:**
- Performance impact of memory ordering
- Correctness across different models
- Speed vs safety trade-offs

### 3. Mutex vs Atomic
Compares traditional synchronization with lock-free programming:
- **Mutex**: Traditional locking mechanism
- **Atomic**: Lock-free operations

**Key Metrics:**
- Performance overhead comparison
- Scalability characteristics
- Contention behavior

## 🎯 Individual Benchmark Folders

Each subfolder contains focused benchmarks for specific scenarios:

### `normal_vs_atomic/`
Dedicated comparison of plain vs atomic operations with detailed analysis.

### `memory_relaxed/`
Benchmarks specifically testing relaxed memory ordering performance.

### `memory_acquire_release/`
Focused on acquire-release semantics and their performance characteristics.

### `memory_sequential/`
Sequential consistency benchmarks for baseline comparison.

## 🔧 Performance Analysis Tips

1. **Multiple Runs**: Run benchmarks multiple times for consistent results
2. **CPU Pinning**: Use `taskset` to bind to specific cores
3. **Frequency Scaling**: Set CPU governor to performance mode
4. **Statistical Analysis**: Collect data over multiple iterations

Example:
```bash
# Run 5 times and average
for i in {1..5}; do
    echo "Run $i:"
    ./comparison
    echo ""
done
```

## 📈 Expected Results

Typical performance hierarchy (fastest to slowest):
1. Plain int (incorrect results)
2. Fetch Add (Relaxed)
3. Fetch Add (Acquire-Release)
4. Fetch Add (Sequentially Consistent)
5. Atomic operations
6. Mutex

## 🧠 Understanding Memory Ordering

- **Relaxed**: No synchronization, only atomicity
- **Acquire-Release**: Synchronizes with release operations
- **Sequentially Consistent**: All operations appear in a single total order

## 🔗 Related Concepts

- Lock-free programming
- Memory barriers
- Cache coherence
- CPU architecture differences
