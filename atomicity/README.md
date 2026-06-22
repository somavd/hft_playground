# Atomicity Demo: Plain vs Mutex

Demonstrates why atomicity is required in multithreaded programs by comparing unsynchronized plain operations with mutex-protected operations.

## Files

- `plain_vs_mutex.cpp` - Comparison of plain counter (data race) vs mutex-protected counter
- `README.md` - This file

## Compile and Run

**To see the data race bug (recommended):**
```bash
cd atomicity
g++ -O0 -std=c++20 -pthread plain_vs_mutex.cpp -o plain_vs_mutex
./plain_vs_mutex
```

**With optimization (may accidentally "fix" the bug):**
```bash
g++ -O3 -std=c++20 -pthread plain_vs_mutex.cpp -o plain_vs_mutex
./plain_vs_mutex
```

## Expected Output

**With `-O0` (no optimization):**
```
=== Atomicity Demo: Plain vs Mutex ===
Threads: 8, Iterations per thread: 1000000
Expected result: 8000000

Plain Counter (no synchronization):
  Result: 1237405 (Expected: 8000000)
  Time: 21 ms
  Status: WRONG - Data race!

Mutex Counter (synchronized):
  Result: 8000000 (Expected: 8000000)
  Time: 301 ms
  Status: CORRECT
```

**With `-O3` (optimization):**
```
=== Atomicity Demo: Plain vs Mutex ===
Threads: 8, Iterations per thread: 1000000
Expected result: 8000000

Plain Counter (no synchronization):
  Result: 8000000 (Expected: 8000000)
  Time: 1 ms
  Status: CORRECT

Mutex Counter (synchronized):
  Result: 8000000 (Expected: 8000000)
  Time: 298 ms
  Status: CORRECT
```

**Note:** With `-O3`, the plain counter may show the "correct" result because the compiler optimizes the expanded read-modify-write into a single atomic instruction. This is **undefined behavior** — the code is still buggy, but the compiler accidentally "fixes" it. For demonstration purposes, use `-O0` to see the actual data race bug.

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
        int temp = plain_counter;  // Read
        temp++;                    // Modify
        plain_counter = temp;      // Write (expanded to increase race window)
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
        int temp = mutex_counter;
        temp++;
        mutex_counter = temp;
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
