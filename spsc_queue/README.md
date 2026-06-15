# SPSC Queue Implementation

This folder contains a lock-free Single Producer Single Consumer (SPSC) queue implementation using C++ templates, commonly used in high-frequency trading applications for ultra-low latency communication between threads.

## Files

- `spsc_queue.cpp` - Template-based SPSC queue implementation with example usage
- `README.md` - This file

## Compile and Run

```bash
cd spsc_queue
g++ -O3 -std=c++20 -pthread spsc_queue.cpp -o spsc_queue
./spsc_queue
```

## Template Implementation Explained

### Template Declaration
```cpp
template<typename T, size_t SIZE>
class SPSCQueue {
```

**What this means:**
- `typename T` - A type parameter that can be any data type (int, struct, class, etc.)
- `size_t SIZE` - A non-type template parameter for compile-time size specification
- The compiler generates a specialized version of this class for each unique combination of T and SIZE

### How Templates Work in This Code

#### 1. **Compile-Time Code Generation**
When you write `SPSCQueue<Tick, 1024> queue;`, the compiler:
- Generates a specialized version of `SPSCQueue` for type `Tick` and size `1024`
- Replaces all instances of `T` with `Tick`
- Replaces all instances of `SIZE` with `1024`
- This happens at compile time, not runtime

#### 2. **Type Safety**
Templates provide compile-time type safety:
```cpp
SPSCQueue<Tick, 1024> queue;      // Queue for Tick objects
SPSCQueue<int, 512> int_queue;    // Queue for integers
SPSCQueue<double, 256> dbl_queue; // Queue for doubles
```
Each instantiation is type-safe - you can't accidentally push a `double` into a `Tick` queue.

#### 3. **Zero Runtime Overhead**
Template instantiation has no runtime cost:
- No virtual function calls
- No type checking at runtime
- Same performance as hand-written specialized code
- The compiler can optimize aggressively

#### 4. **Template Usage in the Code**

**Buffer Declaration:**
```cpp
std::array<T, SIZE> buff;
```
- `std::array` is also a template
- Creates an array of type `T` with `SIZE` elements
- Compile-time fixed size, no dynamic allocation

**Method Parameters:**
```cpp
bool push(const T &item) {
    // item is of type T (whatever was specified)
    buff[current_tail] = item;  // Type-safe assignment
}
```

**Queue Instantiation:**
```cpp
struct Tick {
    int price;
};
SPSCQueue<Tick, 1024> queue;  // Instantiates SPSCQueue for Tick with 1024 slots
```

## SPSC Queue Architecture

### Data Structure
```cpp
std::array<T, SIZE> buff;        // Circular buffer
std::atomic<int> head {0};       // Read position (consumer)
std::atomic<int> tail {0};       // Write position (producer)
```

### Circular Buffer Pattern
- **head**: Points to the next item to be consumed
- **tail**: Points to the next slot to write to
- **Circular**: Both indices wrap around using modulo: `(index + 1) % SIZE`

### Memory Ordering
```cpp
// Producer (push)
tail.load(std::memory_order_relaxed)      // Read tail position
head.load(std::memory_order_acquire)      // Check if full
tail.store(next_tail, std::memory_order_release)  // Update tail

// Consumer (pop)
head.load(std::memory_order_relaxed)      // Read head position
tail.load(std::memory_order_acquire)      // Check if empty
head.store(next_head, std::memory_order_release)  // Update head
```

**Why these memory orders:**
- **relaxed**: For reading own position (no synchronization needed)
- **acquire**: For reading other thread's position (synchronization)
- **release**: For updating own position (synchronization)

### Thread Safety Guarantees
- **Single Producer**: Only one thread calls `push()`
- **Single Consumer**: Only one thread calls `pop()`
- **Lock-free**: No mutexes or locks
- **Wait-free**: Bounded number of operations

## Code Walkthrough

### Push Operation (Producer)
```cpp
bool push(const T &item) {
    int current_tail = tail.load(std::memory_order_relaxed);
    int next_tail = (current_tail + 1) % SIZE;
    
    // Check if queue is full
    if (next_tail == head.load(std::memory_order_acquire)) {
        return false; // Queue is full
    }
    
    buff[current_tail] = item;  // Write data
    tail.store(next_tail, std::memory_order_release);  // Publish
    return true;
}
```

**Steps:**
1. Read current tail position (relaxed - no sync needed)
2. Calculate next position with wrap-around
3. Check if queue is full (acquire - sync with consumer)
4. Write data to buffer
5. Update tail position (release - publish to consumer)

### Pop Operation (Consumer)
```cpp
bool pop(T &item) {
    int current_head = head.load(std::memory_order_relaxed);
    
    // Check if queue is empty
    if (current_head == tail.load(std::memory_order_acquire)) {
        return false; // Queue is empty
    }
    
    item = buff[current_head];  // Read data
    head.store((current_head + 1) % SIZE, std::memory_order_release);  // Update
    return true;
}
```

**Steps:**
1. Read current head position (relaxed - no sync needed)
2. Check if queue is empty (acquire - sync with producer)
3. Read data from buffer
4. Update head position (release - publish to producer)

## Example Usage

### Custom Data Type
```cpp
struct Tick {
    int price;
    int volume;
    uint64_t timestamp;
};

SPSCQueue<Tick, 1024> queue;  // Queue for 1024 Tick objects
```

### Built-in Types
```cpp
SPSCQueue<int, 512> int_queue;      // Queue for integers
SPSCQueue<double, 256> dbl_queue;   // Queue for doubles
SPSCQueue<char, 128> char_queue;    // Queue for characters
```

### Complex Types
```cpp
struct Order {
    int order_id;
    double price;
    int quantity;
    char symbol[16];
};

SPSCQueue<Order, 2048> order_queue;  // Queue for complex orders
```

## Template Advantages in HFT

### 1. **Type Safety**
- Compile-time type checking prevents errors
- No runtime type conversions
- Clear API contracts

### 2. **Performance**
- Zero abstraction overhead
- Compiler optimizations enabled
- Inline-friendly code

### 3. **Flexibility**
- One implementation works for any type
- Easy to add new data types
- No code duplication

### 4. **Maintainability**
- Single source of truth
- Changes propagate to all instantiations
- Easier to debug and test

## Performance Characteristics

### Latency
- **Push**: ~10-50 nanoseconds (depends on CPU)
- **Pop**: ~10-50 nanoseconds (depends on CPU)
- **Cache-friendly**: Sequential memory access

### Throughput
- **Millions of operations per second**
- Limited by memory bandwidth
- No contention (single producer/consumer)

### Memory Usage
- **Fixed size**: `sizeof(T) * SIZE` bytes
- **No dynamic allocation**
- **Cache-line aligned** (with proper padding)

## HFT Use Cases

### Market Data Distribution
```cpp
struct MarketData {
    uint64_t timestamp;
    uint64_t symbol_id;
    double bid_price;
    double ask_price;
    int bid_size;
    int ask_size;
};

SPSCQueue<MarketData, 4096> market_data_queue;
```

### Order Routing
```cpp
struct Order {
    uint64_t order_id;
    char symbol[16];
    double price;
    int quantity;
    char side;  // 'B' or 'S'
};

SPSCQueue<Order, 1024> order_queue;
```

### Risk Management
```cpp
struct PositionUpdate {
    uint64_t account_id;
    uint64_t symbol_id;
    int quantity;
    double avg_price;
};

SPSCQueue<PositionUpdate, 512> position_queue;
```

## Common Pitfalls

### 1. **Multiple Producers/Consumers**
This implementation is NOT thread-safe for:
- Multiple producers calling `push()`
- Multiple consumers calling `pop()`
- Use MPSC or MPMC queues for those cases

### 2. **Queue Full/Empty Handling**
The current implementation spins when full/empty:
```cpp
while(!queue.push(tick)) {
    // Spin - can cause high CPU usage
}
```
Better to add backoff or use condition variables for production use.

### 3. **Size Selection**
- Too small: Frequent full queue, data loss
- Too large: Cache pollution, memory waste
- Choose based on expected burst size

### 4. **Type Requirements**
The type `T` must be:
- Copyable (for `buff[current_tail] = item`)
- Default constructible (for `T item` in pop)
- Not require complex synchronization

## Related Concepts

- **Lock-free programming**: No mutexes or locks
- **Memory ordering**: Acquire-release semantics
- **Circular buffers**: Efficient data structure
- **Template metaprogramming**: Compile-time code generation
- **Cache coherence**: CPU cache behavior
- **False sharing**: Cache line contention

## Further Reading

- [C++ Templates: The Complete Guide](https://en.cppreference.com/w/cpp/language/templates)
- [Lock-Free Programming](https://preshing.com/20120612/an-introduction-to-lock-free-programming/)
- [Memory Ordering in C++](https://preshing.com/20120913/acquire-and-release-semantics/)
- [HFT Programming Best Practices](https://www.highfrequencytrading.com/)
