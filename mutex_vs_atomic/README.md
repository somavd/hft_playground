# Mutex vs Atomic Performance Comparison

Compares the performance difference between mutex-based synchronization and lock-free atomic operations for protecting a shared counter. Both provide correct results, but atomic is faster for simple operations.

## Files

- `mutex_vs_atomic.cpp` - Performance comparison between mutex and atomic counters
- `README.md` - This file

## Compile and Run

```bash
cd mutex_vs_atomic
g++ -O3 -std=c++20 -pthread mutex_vs_atomic.cpp -o mutex_vs_atomic
./mutex_vs_atomic
```

## Expected Output

```
=== Mutex vs Atomic Performance Comparison ===
Threads: 2, Iterations per thread: 10000000
Expected result: 20000000

Mutex-based counter:
  Result: 20000000 (Expected: 20000000)
  Time: 167 ms
  Status: CORRECT

Atomic counter:
  Result: 20000000 (Expected: 20000000)
  Time: 58 ms
  Status: CORRECT

=== Summary ===
Atomic is 2.88x faster than mutex
Both provide correct results, but atomic is faster for simple operations
```

## What is Mutex?

**Mutex (Mutual Exclusion)** is a synchronization primitive that ensures only one thread can access a shared resource at a time.

**How it works:**
```cpp
std::mutex mtx;
std::lock_guard<std::mutex> lock(mtx);
counter++;  // Protected by mutex
```

**Characteristics:**
- Uses OS kernel synchronization (futex on Linux)
- Can block threads (context switch overhead)
- Protects entire critical sections
- Works for complex operations

## What is Atomic?

**Atomic operations** are CPU-level instructions that perform read-modify-write operations atomically without locks.

**How it works:**
```cpp
std::atomic<int> counter{0};
counter++;  // Single atomic instruction
```

**Characteristics:**
- Uses CPU atomic instructions (e.g., `lock xadd` on x86)
- Never blocks (lock-free)
- Only works on single variables
- Faster for simple operations

## Performance Analysis

| Metric | Mutex | Atomic |
|--------|-------|--------|
| **Correctness** | ✅ Correct | ✅ Correct |
| **Time** | ~150-170 ms | ~60-70 ms |
| **Speedup** | 1x | 2-3x faster |
| **Mechanism** | OS-level locks | CPU-level instructions |
| **Blocking** | Can block | Never blocks |

### Why Atomic is Faster

1. **No kernel involvement** — atomic instructions stay in user space
2. **No context switches** — threads never block
3. **Single instruction** — `lock xadd` vs lock → check → unlock
4. **Cache-friendly** — no cache line bouncing for uncontended case

### Why Mutex is Slower

1. **Kernel calls** — lock/unlock may involve syscalls
2. **Context switches** — threads may sleep and wake up
3. **Cache invalidation** — mutex state must be synchronized
4. **Overhead** — lock acquisition and release costs

## When to Use Mutex

**Use mutex when:**
- Protecting complex critical sections (multiple statements)
- Multiple variables must be updated together
- Need to call functions that might block
- Complex invariants must be maintained

**Example:**
```cpp
std::lock_guard<std::mutex> lock(mtx);
balance -= amount;           // Must be atomic with log
log_transaction(amount);     // Can't do this with atomic alone
```

## When to Use Atomic

**Use atomic when:**
- Protecting a single variable
- Simple operations (++, --, load, store, exchange)
- Need lock-free performance
- High-frequency operations

**Example:**
```cpp
std::atomic<int> counter{0};
counter.fetch_add(1, std::memory_order_relaxed);  // Fast increment
```

## Key Takeaways

1. **Both provide correctness** — mutex and atomic both prevent data races
2. **Atomic is faster for simple cases** — 2-3x speedup for single variable operations
3. **Mutex is more flexible** — protects complex critical sections
4. **Choose based on use case** — atomic for simple, mutex for complex
5. **In HFT** — prefer lock-free (atomic) when possible for latency

## Relationship to Atomicity Demo

This folder complements the `atomicity/` folder:
- **atomicity/** — Demonstrates why synchronization is required (plain vs mutex)
- **mutex_vs_atomic/** — Compares two synchronization approaches (mutex vs atomic)

Both show that synchronization is necessary, but this one shows that **how** you synchronize matters for performance.
