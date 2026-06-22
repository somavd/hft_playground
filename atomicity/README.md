# Atomicity Demo: Plain vs Mutex

Demonstrates why atomicity is required in multithreaded programs by comparing unsynchronized plain operations with mutex-protected operations.

## Files

- `plain_vs_mutex.cpp` - Comparison of plain counter (data race) vs mutex-protected counter
- `README.md` - This file

## Compile and Run

```bash
cd atomicity
g++ -O3 -std=c++20 -pthread plain_vs_mutex.cpp -o plain_vs_mutex
./plain_vs_mutex
```

## Expected Output

```
=== Atomicity Demo: Plain vs Mutex ===
Threads: 2, Iterations per thread: 10000000
Expected result: 20000000

Plain Counter (no synchronization):
  Result: 12345678 (Expected: 20000000)
  Time: 15 ms
  Status: WRONG - Data race!

Mutex Counter (synchronized):
  Result: 20000000 (Expected: 20000000)
  Time: 120 ms
  Status: CORRECT

=== Summary ===
Plain counter: Data race bug
Mutex counter: Always correct (atomicity guaranteed)
Performance overhead: 120ms vs 15ms
```

## What is Atomicity?

**Atomicity** means an operation is indivisible — it either completes entirely or not at all. No other thread can see a partially completed state.

In multithreading, atomicity ensures that:
1. Operations complete without interruption
2. Other threads see consistent state
3. No intermediate states are visible

## The Problem: Data Race

### Plain Counter (No Atomicity)

```cpp
int plain_counter = 0;

void plain_worker() {
    for (int i = 0; i < ITERATIONS; ++i) {
        plain_counter++;  // Data race!
    }
}
```

**Why this fails:**

`plain_counter++` is **not atomic**. It compiles to three separate instructions:

```
1. LOAD  eax, [plain_counter]   ; Read value from memory
2. INC   eax                     ; Increment in register
3. STORE [plain_counter], eax    ; Write back to memory
```

**Thread interleaving example:**

| Time | Thread 1 | Thread 2 | Memory Value |
|------|----------|----------|--------------|
| T1   | LOAD 100 | -        | 100 |
| T2   | -        | LOAD 100 | 100 |
| T3   | INC → 101 | -        | 100 |
| T4   | -        | INC → 101 | 100 |
| T5   | STORE 101 | -        | 101 |
| T6   | -        | STORE 101 | 101 |

**Result:** Both threads incremented, but counter only went from 100 → 101 instead of 100 → 102. **One increment was lost.**

This is a **data race** — undefined behavior in C++. The result is unpredictable and may vary between runs.

## The Solution: Mutex

### Mutex-Protected Counter

```cpp
int mutex_counter = 0;
std::mutex mtx;

void mutex_worker() {
    for (int i = 0; i < ITERATIONS; ++i) {
        std::lock_guard<std::mutex> lock(mtx);
        mutex_counter++;  // Protected by mutex
    }
}
```

**How it works:**

The mutex ensures that only one thread can execute the critical section at a time:

```
Thread 1: lock() → increment → unlock()
Thread 2:          wait... → lock() → increment → unlock()
```

**Result:** All increments are preserved. Counter always reaches the expected value.

## Performance Trade-off

| Metric | Plain (Data Race) | Mutex (Synchronized) |
|--------|-------------------|----------------------|
| **Correctness** | Wrong (data race) | Always correct |
| **Performance** | Fast (no synchronization) | Slower (locking overhead) |
| **Use case** | Never (bug) | When correctness matters |

**Why mutex is slower:**
- Lock/unlock involves OS kernel calls
- Threads may block and context switch
- Cache line invalidation between cores

**Typical overhead:** 5-10x slower than unsynchronized code, but **correctness is non-negotiable**.

## When to Use Mutex

**Use mutex when:**
- Multiple threads access shared data
- Operations are not naturally atomic
- You need to protect complex critical sections
- Multiple variables must be updated together

**Example:**
```cpp
std::lock_guard<std::mutex> lock(mtx);
balance -= amount;           // Must be atomic with log
log_transaction(amount);     // Can't do this with atomic alone
```


## Key Takeaways

1. **Plain operations on shared data are unsafe** — data races cause undefined behavior
2. **Mutex provides atomicity** — ensures operations complete without interference
3. **Correctness > Performance** — a fast wrong answer is useless
4. **Use atomic for simple cases** — faster than mutex for single variables
5. **Use mutex for complex cases** — protects entire critical sections
