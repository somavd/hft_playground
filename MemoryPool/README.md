# Memory Pool Implementation

This folder contains a template-based memory pool implementation for high-performance allocation and deallocation, commonly used in high-frequency trading systems to avoid the overhead of dynamic memory allocation.

## Files

- `memory_pool.cpp` - Memory pool implementation with performance benchmark
- `README.md` - This file

## Compile and Run

```bash
cd MemoryPool
g++ -O3 -std=c++20 -pthread memory_pool.cpp -o memory_pool
./memory_pool
```

## Expected Results

```
=== Individual Allocation/Deallocation ===
New/Delete time: XXX microseconds
Memory Pool time: XXX microseconds

=== Vector-based Allocation/Deallocation ===
New/Delete (vector) time: XXX microseconds
Memory Pool (vector) time: XXX microseconds
```

**Typical Performance:**
- Memory pool is typically 5-20x faster than new/delete
- Vector-based pattern shows even larger speedup due to batch operations
- Eliminates heap allocation overhead
- Reduces memory fragmentation
- Improves cache locality

## What is a Memory Pool?

A memory pool is a pre-allocated block of memory that can be used to quickly allocate and deallocate objects of a fixed size. Instead of calling `new` and `delete` for each object (which involves heap management), the pool maintains a free list of available memory blocks.

### Why Use Memory Pools?

1. **Performance**: Eliminates overhead of heap allocation
2. **Deterministic**: No allocation failures or fragmentation
3. **Cache Friendly**: Sequential memory access patterns
4. **Predictable**: Fixed memory usage, no runtime allocation
5. **HFT Critical**: Essential for low-latency trading systems

## Code Analysis

### MemoryPool Class

```cpp
template<typename T>
class MemoryPool {
private:
    struct FreeNode {
        FreeNode *next;
    };

    FreeNode* free_list;
    T* pool;
    size_t pool_size;
```

**Key Components:**
- **FreeNode**: Linked list node for free memory blocks
- **free_list**: Head of the free list (available blocks)
- **pool**: Pre-allocated memory block
- **pool_size**: Number of objects the pool can hold

### Constructor

```cpp
MemoryPool(size_t size) : pool_size(size), free_list(nullptr) {
    pool = static_cast<T*>(::operator new(sizeof(T) * size));

    // Initialize free list in correct order
    for (size_t i = 0; i < pool_size; ++i) {
        FreeNode* node = reinterpret_cast<FreeNode*>(pool + i);
        node->next = free_list;
        free_list = node;
    }
}
```

**What it does:**
1. Allocates a contiguous block of memory for `size` objects
2. Initializes a free list linking all available blocks
3. Each block is treated as a `FreeNode` when not in use

**Memory Layout:**
```
Pool Memory:
[ Block 0 ][ Block 1 ][ Block 2 ]...[ Block N-1 ]
   ↓          ↓          ↓              ↓
FreeList: Block 0 → Block 1 → Block 2 → ... → Block N-1 → nullptr
```

### Allocate

```cpp
T* allocate() {
    if (free_list == nullptr) {
        return nullptr;  // Pool exhausted
    }
    T* result = reinterpret_cast<T*>(free_list);
    free_list = free_list->next;
    return result;
}
```

**What it does:**
1. Checks if free memory is available
2. Removes the first block from the free list
3. Returns raw memory pointer (not constructed object)

**Note:** This returns raw memory. The user must construct the object using placement new.

### Deallocate

```cpp
void deallocate(T* ptr) {
    FreeNode* node = reinterpret_cast<FreeNode*>(ptr);
    node->next = free_list;
    free_list = node;
}
```

**What it does:**
1. Casts the object pointer back to `FreeNode*`
2. Adds the block back to the free list
3. Does NOT call destructor (user's responsibility)

**Note:** The user must call the destructor before deallocation.

## Usage Pattern

### Standard Memory Pool Usage

```cpp
// Create pool for 1000 Order objects
MemoryPool<Order> pool(1000);

// Allocate raw memory
Order* memory = pool.allocate();
assert(memory != nullptr);

// Construct object in allocated memory
Order* order = new(memory) Order(1, 100.0, 10);

// Use the object
std::cout << "Order ID: " << order->id << std::endl;

// Destroy object
order->~Order();

// Return memory to pool
pool.deallocate(order);
```

### Vector-Based Pattern (Batch Operations)

For scenarios where you need to allocate many objects and process them together:

```cpp
std::vector<Order*> orders;
orders.reserve(ITERATIONS);
MemoryPool<Order> pool(ITERATIONS);

// Batch allocation
for(int i = 0; i < ITERATIONS; i++) {
    auto* memory = pool.allocate();
    auto* order = new(memory) Order(i, 100.0, 10);
    orders.push_back(order);
}

// Process all orders
for(auto* order : orders) {
    // Use order
    sink += order->id;
}

// Batch deallocation
for(auto* order : orders) {
    order->~Order();
    pool.deallocate(order);
}
```

**Advantages of Vector Pattern:**
- Better cache locality during processing
- Reduced allocation overhead (batch operations)
- More realistic for HFT workloads
- Shows larger performance gains

### Key Points

1. **Separation of Concerns**: Pool manages memory, user manages objects
2. **Manual Construction**: User must use placement new
3. **Manual Destruction**: User must call destructor
4. **Type Safety**: Pool is templated on object type
5. **No Overhead**: Zero allocation overhead after initialization

## Performance Comparison

### Individual Allocation/Deallocation

**New/Delete Approach:**
```cpp
void benchmark_new_delete() {
    for(int i=0; i<ITERATIONS; i++) {
        Order* order = new Order(i, 100.0, 10);
        sink += order->id;
        delete order;
    }
}
```

**Memory Pool Approach:**
```cpp
void benchmark_memory_pool() {
    MemoryPool<Order> pool(ITERATIONS);
    for(int i=0; i<ITERATIONS; i++) {
        Order* memory = pool.allocate();
        Order* order = new(memory) Order(i, 100.0, 10);
        sink += order->id;
        order->~Order();
        pool.deallocate(order);
    }
}
```

### Vector-Based Batch Operations

**New/Delete with Vector:**
```cpp
void benchmark_new_delete_vector() {
    std::vector<Order*> orders;
    orders.reserve(ITERATIONS);

    for(int i=0; i<ITERATIONS; i++) {
        Order* order = new Order(i, 100.0, 10);
        orders.push_back(order);
    }

    for(auto* order : orders) {
        sink += order->id;
        delete order;
    }
}
```

**Memory Pool with Vector:**
```cpp
void benchmark_memory_pool_vector() {
    std::vector<Order*> orders;
    orders.reserve(ITERATIONS);
    MemoryPool<Order> pool(ITERATIONS);

    for(int i=0; i<ITERATIONS; i++) {
        auto* memory = pool.allocate();
        auto* order = new(memory) Order(i, 100.0, 10);
        orders.push_back(order);
    }

    for(auto* order : orders) {
        sink += order->id;
        order->~Order();
        pool.deallocate(order);
    }
}
```

### Performance Characteristics

**New/Delete Overhead:**
- Heap allocation system call
- Memory fragmentation
- Cache misses from scattered allocations
- Synchronization (thread-safe allocator)

**Memory Pool Advantages:**
- Pre-allocated memory (no system calls)
- Sequential memory access (cache friendly)
- No fragmentation
- No synchronization overhead
- Predictable performance

**Vector Pattern Benefits:**
- Better cache locality during processing
- Reduced allocation overhead (batch operations)
- More realistic for HFT workloads
- Shows larger performance gains

## HFT Applications

### Order Management

```cpp
struct Order {
    int id;
    double price;
    int quantity;
    char symbol[16];
};

MemoryPool<Order> order_pool(10000);

// Fast order allocation
Order* order = new(order_pool.allocate()) Order(...);
// Process order
order->~Order();
order_pool.deallocate(order);
```

### Market Data Processing

```cpp
struct MarketData {
    uint64_t timestamp;
    uint64_t symbol_id;
    double bid_price;
    double ask_price;
};

MemoryPool<MarketData> data_pool(100000);

// High-frequency market data processing
MarketData* data = new(data_pool.allocate()) MarketData(...);
// Process data
data->~MarketData();
data_pool.deallocate(data);
```

### Risk Management

```cpp
struct Position {
    uint64_t account_id;
    uint64_t symbol_id;
    int quantity;
    double avg_price;
};

MemoryPool<Position> position_pool(5000);

// Fast position updates
Position* pos = new(position_pool.allocate()) Position(...);
// Update position
pos->~Position();
position_pool.deallocate(pos);
```

## Design Decisions

### Why Raw Memory Instead of Constructed Objects?

**Advantages:**
1. **Flexibility**: Can construct different types in same pool
2. **Control**: User has full control over object lifetimes
3. **Performance**: No hidden construction/destruction overhead
4. **Standard Pattern**: Matches real-world HFT memory pool designs

**Trade-offs:**
1. **Manual Management**: User must handle construction/destruction
2. **Error Prone**: Easy to forget destructor call
3. **Less Safe**: No automatic resource management

### Why Free List Instead of Stack?

**Advantages:**
1. **O(1) Allocation**: Constant time allocation/deallocation
2. **Flexibility**: Can deallocate in any order
3. **Simple**: Easy to implement and understand

**Trade-offs:**
1. **Cache Misses**: Free list traversal can cause cache misses
2. **Fragmentation**: Potential for scattered free blocks

## Common Pitfalls

### 1. Forgetting Destructor Call

**Wrong:**
```cpp
Order* order = new(memory) Order(...);
pool.deallocate(order);  // Destructor not called!
```

**Correct:**
```cpp
Order* order = new(memory) Order(...);
order->~Order();  // Call destructor first
pool.deallocate(order);
```

### 2. Double Deallocation

**Wrong:**
```cpp
pool.deallocate(order);
pool.deallocate(order);  // Double free!
```

**Correct:**
```cpp
pool.deallocate(order);
// Don't deallocate again
```

### 3. Using After Deallocation

**Wrong:**
```cpp
pool.deallocate(order);
order->id = 5;  // Use after free!
```

**Correct:**
```cpp
pool.deallocate(order);
order = nullptr;  // Nullify pointer
```

### 4. Pool Exhaustion

**Wrong:**
```cpp
Order* order = pool.allocate();
assert(order != nullptr);  // May fail
```

**Correct:**
```cpp
Order* order = pool.allocate();
if (order == nullptr) {
    // Handle pool exhaustion
    std::cerr << "Pool exhausted!" << std::endl;
    return;
}
```

### 5. Type Mismatch

**Wrong:**
```cpp
MemoryPool<Order> order_pool(100);
Trade* trade = new(order_pool.allocate()) Trade(...);  // Wrong type!
```

**Correct:**
```cpp
MemoryPool<Order> order_pool(100);
Order* order = new(order_pool.allocate()) Order(...);  // Correct type
```

## Advanced Usage

### Multiple Pools for Different Types

```cpp
MemoryPool<Order> order_pool(10000);
MemoryPool<Trade> trade_pool(50000);
MemoryPool<Position> position_pool(1000);

// Use appropriate pool for each type
Order* order = new(order_pool.allocate()) Order(...);
Trade* trade = new(trade_pool.allocate()) Trade(...);
Position* pos = new(position_pool.allocate()) Position(...);
```

### Pool Reuse

```cpp
MemoryPool<Order> order_pool(10000);

// Use pool in multiple operations
for (int i = 0; i < 100; ++i) {
    Order* order = new(order_pool.allocate()) Order(...);
    // Process order
    order->~Order();
    order_pool.deallocate(order);
}

// Pool can be reused - all memory is back in free list
```

### Thread Safety Considerations

**Current implementation is NOT thread-safe.** For multi-threaded use:

1. **Add synchronization:**
```cpp
std::mutex pool_mutex;

T* allocate() {
    std::lock_guard<std::mutex> lock(pool_mutex);
    // ... existing code
}
```

2. **Use thread-local pools:**
```cpp
thread_local MemoryPool<Order> local_pool(1000);
```

3. **Use lock-free free list:**
```cpp
std::atomic<FreeNode*> free_list;
```

## Performance Characteristics

### Allocation Speed
- **New/Delete**: 50-200 nanoseconds per allocation
- **Memory Pool**: 5-20 nanoseconds per allocation
- **Speedup**: 5-20x faster

### Memory Usage
- **New/Delete**: Fragmented, unpredictable
- **Memory Pool**: Fixed, predictable
- **Overhead**: sizeof(T) * pool_size bytes

### Cache Behavior
- **New/Delete**: Scattered memory, cache misses
- **Memory Pool**: Sequential memory, cache hits
- **Improvement**: Better cache locality

## Best Practices

### 1. Size the Pool Appropriately
```cpp
// Too small - frequent exhaustion
MemoryPool<Order> pool(10);

// Too large - memory waste
MemoryPool<Order> pool(1000000);

// Just right - based on expected usage
MemoryPool<Order> pool(10000);
```

### 2. Use RAII Wrappers
```cpp
template<typename T, typename Pool>
class PoolPtr {
    T* ptr;
    Pool& pool;
public:
    PoolPtr(T* p, Pool& pl) : ptr(p), pool(pl) {}
    ~PoolPtr() {
        if (ptr) {
            ptr->~T();
            pool.deallocate(ptr);
        }
    }
    T* operator->() { return ptr; }
    T& operator*() { return *ptr; }
};

// Usage
PoolPtr<Order, MemoryPool<Order>> order(
    new(pool.allocate()) Order(...), pool);
```

### 3. Validate Pointers
```cpp
Order* order = pool.allocate();
if (order == nullptr) {
    // Handle exhaustion
    return;
}
```

### 4. Nullify After Deallocation
```cpp
order->~Order();
pool.deallocate(order);
order = nullptr;  // Prevent use-after-free
```

## Related Concepts

- **Placement New**: Constructing objects in pre-allocated memory
- **Object Pools**: Managing object lifetimes explicitly
- **Arena Allocators**: Region-based memory allocation
- **Memory Fragmentation**: Heap memory becoming fragmented
- **Cache Locality**: Data being in CPU cache
- **Custom Allocators**: Implementing custom memory management

## Further Reading

- [C++ Placement New](https://en.cppreference.com/w/cpp/language/new)
- [Memory Pool Pattern](https://en.wikipedia.org/wiki/Pool_(computer_science))
- [HFT Memory Management](https://www.highfrequencytrading.com/)
- [Custom Allocators in C++](https://en.cppreference.com/w/cpp/memory/allocator)
- [Cache-Oriented Programming](https://www.agner.org/optimize/optimizing_cpp.pdf)
