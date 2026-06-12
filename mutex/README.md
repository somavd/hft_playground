# Mutex vs Atomic Comparison

This benchmark compares the performance between traditional mutex-based synchronization and lock-free atomic operations.

## Compile

```bash
g++ -O3 -std=c++20 -pthread src.cpp -o benchmark
```

## Run

```bash
./benchmark
```

## What it Tests

- **Mutex**: Traditional locking mechanism using std::mutex
- **Atomic**: Lock-free operations using std::atomic

## Key Insights

- Mutex provides traditional locking mechanism
- Atomic provides lock-free operations
- Atomic operations typically outperform mutex
- Mutex overhead comes from kernel-level synchronization
- Atomic operations use CPU-level instructions

## When to use Mutex

- Complex critical sections
- Multiple operations need to be atomic
- Legacy code compatibility

## When to use Atomic

- Simple operations (increment, decrement)
- High-performance requirements
- Lock-free programming
