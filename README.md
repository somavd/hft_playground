# HFT Playground

A comprehensive collection of C++ benchmarks focused on high-frequency trading concepts, demonstrating atomic operations, memory ordering, and synchronization primitives.

## 📁 Project Structure

```
hft_playground/
├── README.md                           # This file
├── atomic/                             # Atomic vs normal counter comparison
│   ├── src.cpp                         # Benchmark implementation
│   └── README.md                       # Build and run instructions
├── mutex/                              # Mutex vs atomic comparison
│   ├── src.cpp                         # Benchmark implementation
│   └── README.md                       # Build and run instructions
├── memory_ordering/                    # Memory ordering examples
│   ├── without_mem_ordering.cpp        # Problems without proper ordering
│   ├── with_memory_ordering.cpp        # Solution with acquire-release
│   └── README.md                       # Detailed explanations
├── types_of_memory_ordering/           # Different memory ordering types
│   ├── memory_acquire_release.cpp      # Acquire-release semantics
│   ├── memory_sequential.cpp           # Sequential consistency
│   ├── memory_relaxed.cpp              # Relaxed ordering
│   └── README.md                       # Comprehensive guide
└── performance_comparison/              # Comprehensive performance suite
    ├── performance_comparison.cpp      # All benchmarks in one file
    └── README.md                       # Performance analysis guide
└── memory_mapped_files/                # Memory-mapped files and shared memory
    ├── basic_mmap.cpp                  # Basic memory-mapped file operations
    ├── shared_memory.cpp               # Inter-process communication
    └── README.md                       # Detailed explanations
```

## 🚀 Quick Start

### Run Individual Benchmarks

#### Atomic vs Normal Counter
```bash
cd atomic
g++ -O3 -std=c++20 -pthread src.cpp -o benchmark
./benchmark
```

#### Mutex vs Atomic
```bash
cd mutex
g++ -O3 -std=c++20 -pthread src.cpp -o benchmark
./benchmark
```

#### Memory Ordering Examples
```bash
cd memory_ordering

# Without proper memory ordering
g++ -O3 -std=c++20 -pthread without_mem_ordering.cpp -o without_mem_ordering
./without_mem_ordering

# With proper memory ordering
g++ -O3 -std=c++20 -pthread with_memory_ordering.cpp -o with_memory_ordering
./with_memory_ordering
```

#### Memory Ordering Types
```bash
cd types_of_memory_ordering

# Acquire-Release
g++ -O3 -std=c++20 -pthread memory_acquire_release.cpp -o memory_acquire_release
./memory_acquire_release

# Sequential Consistency
g++ -O3 -std=c++20 -pthread memory_sequential.cpp -o memory_sequential
./memory_sequential

# Relaxed Ordering
g++ -O3 -std=c++20 -pthread memory_relaxed.cpp -o memory_relaxed
./memory_relaxed
```

#### Comprehensive Performance Suite
```bash
cd performance_comparison
g++ -O3 -std=c++20 -pthread performance_comparison.cpp -o performance_comparison
./performance_comparison
```

#### Memory-Mapped Files
```bash
cd memory_mapped_files

# Basic memory-mapped file operations
g++ -O3 -std=c++20 -pthread basic_mmap.cpp -o basic_mmap
./basic_mmap

# Shared memory demonstration
g++ -O3 -std=c++20 -pthread shared_memory.cpp -o shared_memory
./shared_memory
```

## 📊 Benchmark Categories

### 1. **Atomic Operations** (`atomic/`)
- Plain int vs atomic counter comparison
- Demonstrates data races and thread safety
- Performance impact of atomic operations

### 2. **Mutex vs Atomic** (`mutex/`)
- Traditional locking vs lock-free programming
- Performance comparison of synchronization methods
- When to use each approach

### 3. **Memory Ordering Basics** (`memory_ordering/`)
- Problems without proper memory ordering
- Solutions with acquire-release semantics
- Producer-consumer synchronization

### 4. **Memory Ordering Types** (`types_of_memory_ordering/`)
- **Relaxed**: Fastest, only atomicity
- **Acquire-Release**: Synchronization without full consistency
- **Sequentially Consistent**: Strongest guarantees

### 5. **Performance Comparison** (`performance_comparison/`)
- All benchmarks in one comprehensive suite
- Side-by-side performance analysis
- Detailed timing comparisons

### 6. **Memory-Mapped Files** (`memory_mapped_files/`)
- Zero-copy I/O operations
- Inter-process communication via shared memory
- High-performance file access patterns
- HFT-specific use cases

## 📚 Concepts Covered

### Atomic Operations
- Thread-safe operations without locks
- Performance characteristics
- Memory ordering implications

### Memory Ordering
- **Relaxed**: No ordering guarantees, fastest
- **Acquire-Release**: Synchronization between threads
- **Sequentially Consistent**: Global total order

### Synchronization
- **Mutex**: Traditional locking mechanism
- **Atomic**: Lock-free operations
- Performance trade-offs

### Data Races
- Understanding undefined behavior
- Importance of proper synchronization
- Real-world implications

### Memory-Mapped Files
- Zero-copy I/O operations
- Shared memory for inter-process communication
- High-performance file access
- HFT market data distribution

## 🔧 Performance Analysis

For detailed performance analysis:

```bash
# Use perf for detailed metrics
perf stat ./benchmark

# Profile with detailed instructions
perf record -g ./benchmark
perf report

# Multiple runs for consistency
for i in {1..5}; do
    echo "Run $i:"
    ./benchmark
done
```

## 🎯 Use Cases

This playground is ideal for:
- Learning atomic operations and memory ordering
- Understanding HFT performance optimization
- Benchmarking synchronization primitives
- Teaching concurrent programming concepts
- Research in lock-free algorithms

## 📈 Expected Performance Hierarchy

Typical performance (fastest to slowest):
1. Plain int (incorrect, data race)
2. Relaxed atomic operations
3. Acquire-Release operations
4. Sequentially consistent operations
5. Mutex operations

## 🔗 Related Resources

- [C++ Reference: std::atomic](https://en.cppreference.com/w/cpp/atomic)
- [Memory Ordering in C++](https://preshing.com/20120913/acquire-and-release-semantics/)
- [HFT Programming Best Practices](https://www.highfrequencytrading.com/)

## 🤝 Contributing

Each benchmark is self-contained. Feel free to:
- Add new benchmarks
- Improve existing ones
- Update documentation
- Share performance insights
