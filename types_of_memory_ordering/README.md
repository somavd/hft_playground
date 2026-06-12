# Types of Memory Ordering

This folder demonstrates the different memory ordering types available in C++ atomic operations, showing how each one works and when to use them.

## Files

- `memory_acquire_release.cpp` - Demonstrates acquire-release semantics
- `memory_sequential.cpp` - Demonstrates sequentially consistent ordering  
- `memory_relaxed.cpp` - Demonstrates relaxed memory ordering

## Compile and Run

```bash
# Compile all examples
g++ -O3 -std=c++20 -pthread memory_acquire_release.cpp -o memory_acquire_release
g++ -O3 -std=c++20 -pthread memory_sequential.cpp -o memory_sequential
g++ -O3 -std=c++20 -pthread memory_relaxed.cpp -o memory_relaxed

# Run each example
./memory_acquire_release
./memory_sequential  
./memory_relaxed
```

## Memory Ordering Types

### 1. Memory Order Acquire-Release (`memory_acquire_release.cpp`)

**How it works:**
- **Release**: Ensures all writes before this operation are visible to other threads
- **Acquire**: Ensures all reads after this operation see the latest writes
- Forms a synchronization pair between threads

**When to use:**
- Producer-consumer patterns
- Synchronizing data between threads
- When you need ordering but not full consistency

**Key characteristics:**
- Prevents reordering across the acquire-release barrier
- Establishes happens-before relationship
- More efficient than sequential consistency
- Still provides strong synchronization guarantees

### 2. Memory Order Sequentially Consistent (`memory_sequential.cpp`)

**How it works:**
- All atomic operations appear in a single global order
- All threads agree on the order of all operations
- Prevents all reordering of atomic operations

**When to use:**
- When you need the strongest guarantees
- Default choice for most applications
- When correctness is more important than performance

**Key characteristics:**
- Default memory order for atomic operations
- Easiest to reason about
- Potentially slower than other orders
- Guarantees total order across all threads

### 3. Memory Order Relaxed (`memory_relaxed.cpp`)

**How it works:**
- Only provides atomicity (no reordering guarantees)
- Operations can be reordered by compiler or CPU
- No synchronization between threads

**When to use:**
- Simple counters where ordering doesn't matter
- Performance-critical code where ordering isn't needed
- When only atomicity is required

**Key characteristics:**
- Fastest atomic operation
- Only guarantees atomicity
- No ordering or synchronization
- Dangerous for inter-thread communication

## Performance Comparison

Typical performance (fastest to slowest):
1. **Relaxed** - Fastest, minimal overhead
2. **Acquire-Release** - Moderate overhead
3. **Sequentially Consistent** - Slowest, strongest guarantees

## Usage Guidelines

### Choose Relaxed when:
- You only need atomicity
- Ordering between operations doesn't matter
- Performance is critical
- Example: Simple counters

### Choose Acquire-Release when:
- You need synchronization between threads
- Producer-consumer patterns
- You need some ordering but not full consistency
- Example: Flag-based synchronization

### Choose Sequentially Consistent when:
- You need the strongest guarantees
- Correctness is paramount
- You're unsure what to use
- Example: Complex synchronization scenarios

## Common Pitfalls

1. **Using relaxed for synchronization** - Can lead to subtle bugs
2. **Overusing sequential consistency** - May hurt performance unnecessarily
3. **Forgetting acquire-release pairing** - Breaks synchronization
4. **Mixing memory orders incorrectly** - Can cause undefined behavior

## Related Concepts

- Atomic operations and their guarantees
- Happens-before relationships
- Memory barriers and fences
- CPU cache coherence
- Compiler optimization and reordering
