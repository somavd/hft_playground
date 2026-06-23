# SPSC Queue with Atomics (Version 2 - Lock-Free)

Lock-free Single Producer Single Consumer queue using atomic operations and a circular buffer, replacing the mutex-based version 1.

## Files

- `spsc_atomic.cpp` - Lock-free SPSC queue with atomic head/tail
- `README.md` - This file

## Compile and Run

```bash
cd Projects/SPSC/version2
g++ -O3 -std=c++20 -pthread spsc_atomic.cpp -o spsc_atomic
./spsc_atomic
```

## Version 1 (Mutex) vs Version 2 (Atomic)

| Feature | Version 1 (Mutex) | Version 2 (Atomic) |
|---------|-------------------|---------------------|
| **Synchronization** | `std::mutex` | `std::atomic` |
| **Buffer** | Circular buffer (std::vector) | Circular buffer (std::vector) |
| **Lock-free** | No | Yes |
| **System calls** | Yes (futex on contention) | No |
| **Memory allocation** | Pre-allocated | Pre-allocated |
| **False sharing** | N/A (mutex on separate line) | `alignas(64)` on head/tail |
| **Throughput** | ~6M msg/s | ~25M msg/s |
| **Actual benchmark** | 1.67s (6.0M msg/s) | 0.40s (24.8M msg/s) |
| **Speedup** | 1x | **4.15x faster** |

## Code Analysis

### SPSCQueue Class

```cpp
template<typename T>
class SPSCQueue {
private:
    std::vector<T> buffer_;
    alignas(64) std::atomic<size_t> head{0};  // Prevents false sharing
    alignas(64) std::atomic<size_t> tail{0};  // Prevents false sharing
    size_t max_size_;

public:
    SPSCQueue(size_t max_size = 10000) : max_size_(max_size) {
        buffer_.resize(max_size);
    }

    bool push(const T& item) {
        size_t current_tail = tail.load(std::memory_order_relaxed);
        size_t next_tail = (current_tail + 1) % max_size_;
        if (next_tail == head.load(std::memory_order_acquire)) {
            return false; // Queue is full
        }
        buffer_[current_tail] = item;
        tail.store(next_tail, std::memory_order_release);
        return true;
    }

    bool pop(T& item) {
        size_t current_head = head.load(std::memory_order_relaxed);
        if(current_head == tail.load(std::memory_order_acquire)) {
            return false; // Queue is empty
        }
        item = buffer_[current_head];
        head.store((current_head + 1) % max_size_, std::memory_order_release);
        return true;
    }
};
```

**Key Components:**
- **std::vector**: Pre-allocated circular buffer
- **alignas(64) std::atomic**: Atomic head/tail with cache line alignment
- **Memory ordering**: Acquire-release semantics for synchronization
- **Lock-free**: No mutex, uses atomic operations only

**Optimizations:**
- `alignas(64)` prevents false sharing between head and tail
- Acquire-release memory ordering (not seq_cst) for better performance
- Relaxed loads for variables only written by one thread

## Memory Ordering Explained

```cpp
// Producer: push()
tail_.load(std::memory_order_relaxed);    // Only producer writes tail — relaxed is safe
head_.load(std::memory_order_acquire);    // Must see consumer's latest head write
// ... write buffer ...
tail_.store(next, std::memory_order_release);  // Make buffer write visible to consumer

// Consumer: pop()
head_.load(std::memory_order_relaxed);    // Only consumer writes head — relaxed is safe
tail_.load(std::memory_order_acquire);    // Must see producer's latest tail write
// ... read buffer ...
head_.store(next, std::memory_order_release);  // Make buffer read complete before advancing
```

**Why not seq_cst?** Sequential consistency adds full memory barriers (~10-50 cycles). Acquire-release is sufficient for SPSC and avoids unnecessary overhead.

## Expected Performance

```
Time taken: 402698167 ns
Throughput: 2.48325e+07 msg/s
```

**Actual Performance:**
- Throughput: ~25 million messages/second
- Time: ~0.40 seconds for 10M messages
- **4.15x faster** than mutex-based version (6M msg/s)

**Why faster:**
- No lock/unlock overhead
- No potential blocking or context switches
- `alignas(64)` prevents false sharing between head and tail
- Acquire-release memory ordering is lighter than seq_cst
- User-space only (no kernel involvement)

## Further Reading

- [Lock-Free Programming](https://preshing.com/20120612/an-introduction-to-lock-free-programming/)
- [False Sharing](https://en.wikipedia.org/wiki/False_sharing)
- [Memory Ordering](https://preshing.com/20120913/acquire-and-release-semantics/)
- [Folly ProducerConsumerQueue](https://github.com/facebook/folly/blob/main/folly/ProducerConsumerQueue.h)
