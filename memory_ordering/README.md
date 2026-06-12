# Memory Ordering Examples

This folder contains examples demonstrating the importance of proper memory ordering in C++ atomic operations through a producer-consumer pattern.

## Files

- `without_mem_ordering.cpp` - Demonstrates issues without proper memory ordering
- `with_memory_ordering.cpp` - Demonstrates correct implementation with memory ordering

## Compile and Run

### Without Memory Ordering
```bash
# Compile
g++ -O3 -std=c++20 -pthread without_mem_ordering.cpp -o without_mem_ordering

# Run
./without_mem_ordering
```

### With Memory Ordering
```bash
# Compile
g++ -O3 -std=c++20 -pthread with_memory_ordering.cpp -o with_memory_ordering

# Run
./with_memory_ordering
```

## Comparison

### `without_mem_ordering.cpp` - The Problem

**Issues Demonstrated:**
1. **Memory Ordering Problem**: 
   - `data = 42` and `ready.store(true)` may be reordered by compiler/CPU
   - Consumer might see `ready = true` but still read `data = 0`

2. **Data Race**:
   - `data` is a plain int accessed by multiple threads
   - This causes undefined behavior

**Expected Behavior:**
- May print incorrect data (0 instead of 42)
- Results are unpredictable between runs
- Demonstrates why proper memory ordering is essential

### `with_memory_ordering.cpp` - The Solution

**Correct Implementation:**
1. **Release Semantics**: `ready.store(true, std::memory_order_release)`
   - Ensures all writes before this point are visible to other threads

2. **Acquire Semantics**: `ready.load(std::memory_order_acquire)`
   - Ensures all reads after this point see the latest writes

**Expected Behavior:**
- Should consistently print correct data (42)
- Proper synchronization between producer and consumer
- Demonstrates correct memory ordering usage

## Key Insights

- **Memory Ordering Matters**: Atomic operations alone don't guarantee correct ordering
- **Acquire-Release Pairing**: Essential for producer-consumer synchronization
- **Data Races**: Still present in both examples (data should be atomic)
- **Real-world Impact**: Without proper ordering, programs can have subtle bugs

## Related Concepts

- Memory ordering (relaxed, acquire, release, seq_cst)
- Producer-consumer synchronization
- Data races and undefined behavior
- Compiler/CPU instruction reordering
- Release-acquire semantics
