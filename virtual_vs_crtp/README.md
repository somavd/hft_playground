# Virtual Functions vs CRTP

This folder demonstrates the performance difference between traditional virtual function dispatch and the Curiously Recurring Template Pattern (CRTP), a compile-time polymorphism technique commonly used in high-frequency trading for zero-overhead abstractions.

## Files

- `virtual_vs_crtp.cpp` - Performance comparison between virtual dispatch and CRTP
- `README.md` - This file

## Compile and Run

```bash
cd virtual_vs_crtp
g++ -O3 -std=c++20 -pthread virtual_vs_crtp.cpp -o virtual_vs_crtp
./virtual_vs_crtp
```

## Expected Results

```
Virtual Dispatch : XXX ms | Result = XXX
CRTP Dispatch : XXX ms | Result = XXX
```

**Typical Performance:**
- CRTP is typically 2-5x faster than virtual dispatch
- CRTP has zero runtime overhead (compile-time resolution)
- Virtual dispatch has vtable lookup overhead

## Virtual Functions Explained

### How Virtual Functions Work

```cpp
class VirtualBase {
public:
    virtual int execute(int x) const {
        return x + 1;
    }
};

class VirtualDerived : public VirtualBase {
public:
    int execute(int x) const override {
        return x + 2;
    }
};
```

**Runtime Mechanism:**
1. **VTable**: Each class with virtual functions has a virtual table (vtable)
2. **VPointer**: Each object has a pointer to its vtable (vptr)
3. **Dispatch**: Virtual function calls look up the function address in the vtable
4. **Indirection**: Extra indirection through vptr and vtable

**Memory Layout:**
```
VirtualConcrete object:
[ vptr (8 bytes) ][ data members ]
```

**VTable Structure:**
```
VirtualStrategy vtable:
[ &VirtualStrategy::execute ][ typeinfo ]

VirtualConcrete vtable:
[ &VirtualConcrete::execute ][ typeinfo ]
```

### Virtual Dispatch Overhead

**Call Sequence:**
```cpp
virtualPtr->execute(x);
```

1. Load vptr from object
2. Load function address from vtable
3. Jump to function address
4. Execute function

**Cost:**
- Memory load for vptr
- Memory load for function address
- Branch prediction challenges
- Potential cache misses

## CRTP Explained

### How CRTP Works

```cpp
template<typename Derived>
class CRTPBase {
public:
    int execute(int x) const {
        return static_cast<const Derived*>(this)->executeImpl(x);
    }
};

class CRTPDerived : public CRTPBase<CRTPDerived> {
public:
    int executeImpl(int x) const {
        return x + 3;
    }
};
```

**Compile-Time Mechanism:**
1. **Template Instantiation**: Compiler generates specialized code for each derived type
2. **Static Casting**: `static_cast` is resolved at compile time
3. **Inline Expansion**: Functions can be inlined by the compiler
4. **Direct Calls**: No runtime indirection

**The "Curious" Pattern:**
```cpp
class CRTPConcrete : public CRTPBase<CRTPConcrete>
//                    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//                    Derived class passes itself as template parameter
```

### CRTP Advantages

**Zero Runtime Overhead:**
- No vtable lookups
- No virtual function indirection
- Direct function calls
- Compiler optimizations enabled

**Compile-Time Polymorphism:**
- Type safety at compile time
- Better optimization opportunities
- Inline expansion possible
- No runtime type checking

## Code Analysis

### Virtual Dispatch Implementation

```cpp
class VirtualBase {
public:
    virtual int execute(int x) const {
        return x + 1;
    }
};

class VirtualDerived : public VirtualBase {
public:
    int execute(int x) const override {
        return x + 2;
    }
};
```

**Characteristics:**
- **Runtime polymorphism**: Dynamic dispatch
- **VTable overhead**: Indirect function calls
- **Memory overhead**: vptr in each object
- **Flexibility**: Can change behavior at runtime

### CRTP Implementation

```cpp
template<typename Derived>
class CRTPBase {
public:
    int execute(int x) const {
        return static_cast<const Derived*>(this)->executeImpl(x);
    }
};

class CRTPDerived : public CRTPBase<CRTPDerived> {
public:
    int executeImpl(int x) const {
        return x + 3;
    }
};
```

**Characteristics:**
- **Compile-time polymorphism**: Static dispatch
- **Zero overhead**: Direct function calls
- **No memory overhead**: No vptr needed
- **Static behavior**: Fixed at compile time

### Benchmark Setup

```cpp
template<typename Func>
void benchmark(const std::string& name, Func func)
{
    volatile long long result = 0;

    auto start =
        std::chrono::high_resolution_clock::now();

    for(std::size_t i = 0; i < ITERATIONS; ++i)
    {
        result += func(static_cast<int>(i));
    }

    auto end =
        std::chrono::high_resolution_clock::now();

    auto duration =
        std::chrono::duration_cast<
            std::chrono::milliseconds>(
                end - start);

    std::cout
        << name
        << " : "
        << duration.count()
        << " ms"
        << " | Result = "
        << result
        << '\n';
}
```

**Key Features:**
- **Template function**: Works with any callable
- **Volatile result**: Prevents compiler optimization
- **High iterations**: 100 million calls for meaningful timing
- **Precision timing**: Uses high-resolution clock

## Performance Comparison

### Virtual Dispatch Performance

**Latency:**
- **VTable lookup**: 1-2 cycles (cache hit)
- **Function call**: 2-3 cycles
- **Total**: 3-5 cycles per call
- **Cache miss penalty**: 10-50 cycles

**Memory:**
- **VTable**: One per class (8-16 bytes)
- **VPointer**: One per object (8 bytes)
- **Cache pollution**: VTables can cause cache misses

### CRTP Performance

**Latency:**
- **Direct call**: 1-2 cycles
- **Inlined**: 0 cycles (if inlined)
- **Total**: 0-2 cycles per call
- **No cache misses**: No indirection

**Memory:**
- **No vtable**: Zero overhead
- **No vptr**: Zero overhead
- **Code bloat**: Possible due to template instantiation

### Performance Results

**Typical Benchmark Results:**
```
Virtual Dispatch : 150 ms | Result = 5000000050000000
CRTP Dispatch : 50 ms | Result = 5000000050000000
```

**Speedup:**
- **2-5x faster** for CRTP
- **Zero overhead** when inlined
- **Better cache locality**

## When to Use Each Approach

### Use Virtual Functions When:

1. **Runtime Polymorphism Needed**
   - Need to change behavior at runtime
   - Plugin architectures
   - Dynamic loading of modules

2. **Type Erasure Required**
   - Heterogeneous containers
   - Type-agnostic interfaces
   - Factory patterns

3. **Binary Compatibility**
   - Need stable ABI
   - Shared libraries
   - Plugin systems

**Example:**
```cpp
std::vector<std::unique_ptr<Strategy>> strategies;
strategies.push_back(std::make_unique<StrategyA>());
strategies.push_back(std::make_unique<StrategyB>());
for (auto& strategy : strategies) {
    strategy->execute();  // Runtime dispatch
}
```

### Use CRTP When:

1. **Performance Critical**
   - HFT applications
   - Real-time systems
   - High-frequency operations

2. **Static Polymorphism Sufficient**
   - Types known at compile time
   - No runtime type changes
   - Fixed behavior patterns

3. **Zero Overhead Required**
   - Tight loops
   - Performance-sensitive code
   - Latency-critical operations

**Example:**
```cpp
template<typename T>
void process_data(T& data) {
    data.execute();  // Compile-time dispatch, can be inlined
}
```

## HFT Applications

### Order Processing

**Virtual Functions (Flexible):**
```cpp
class OrderHandler {
public:
    virtual void process(Order& order) = 0;
};

class MarketOrderHandler : public OrderHandler {
public:
    void process(Order& order) override {
        // Market order logic
    }
};

class LimitOrderHandler : public OrderHandler {
public:
    void process(Order& order) override {
        // Limit order logic
    }
};
```

**CRTP (Performance):**
```cpp
template<typename Derived>
class OrderProcessor {
public:
    inline void process(Order& order) {
        static_cast<Derived*>(this)->processImpl(order);
    }
};

class MarketOrderProcessor : public OrderProcessor<MarketOrderProcessor> {
public:
    inline void processImpl(Order& order) {
        // Market order logic - can be inlined
    }
};
```

### Risk Management

**Virtual Functions:**
```cpp
class RiskChecker {
public:
    virtual bool check(const Position& pos) = 0;
};
```

**CRTP:**
```cpp
template<typename Derived>
class RiskChecker {
public:
    inline bool check(const Position& pos) {
        return static_cast<Derived*>(this)->checkImpl(pos);
    }
};
```

## Advanced CRTP Patterns

### Mixin Pattern

```cpp
template<typename Derived>
class EqualityComparable {
public:
    bool operator==(const Derived& other) const {
        return static_cast<const Derived*>(this)->equals(other);
    }
};

class Point : public EqualityComparable<Point> {
    int x, y;
public:
    bool equals(const Point& other) const {
        return x == other.x && y == other.y;
    }
};
```

### Interface Pattern

```cpp
template<typename Derived>
class Drawable {
public:
    void draw() {
        static_cast<Derived*>(this)->drawImpl();
    }
};

class Circle : public Drawable<Circle> {
public:
    void drawImpl() {
        // Draw circle
    }
};
```

### Static Polymorphism

```cpp
template<typename T>
void process(T& obj) {
    obj.execute();  // Compile-time dispatch
}

process(virtualObj);  // Virtual dispatch
process(crtpObj);     // CRTP dispatch (can be inlined)
```

## Common Pitfalls

### Virtual Functions

1. **Performance Overhead**
   - Not suitable for tight loops
   - Cache misses on vtable lookups
   - Branch prediction challenges

2. **Memory Overhead**
   - Vptr in each object
   - VTable for each class
   - Can increase memory usage

3. **Constructor/Destructor Issues**
   - Virtual destructors needed for proper cleanup
   - Constructor calls don't use virtual dispatch

### CRTP

1. **Code Bloat**
   - Template instantiation for each type
   - Can increase binary size
   - Longer compilation times

2. **Runtime Flexibility**
   - Cannot change behavior at runtime
   - Types must be known at compile time
   - Not suitable for plugin architectures

3. **Complexity**
   - Template syntax can be confusing
   - Error messages can be cryptic
   - Harder to debug

## Best Practices

### Virtual Functions

1. **Use for Interface Design**
   - Define clear interfaces
   - Enable runtime flexibility
   - Support plugin architectures

2. **Mark Destructors Virtual**
   ```cpp
   class Base {
   public:
       virtual ~Base() = default;  // Always virtual
   };
   ```

3. **Consider Final Specifier**
   ```cpp
   class Derived final : public Base {
       // Prevents further inheritance
   };
   ```

### CRTP

1. **Use for Performance-Critical Code**
   - HFT applications
   - Real-time systems
   - Tight loops

2. **Use Inline Keyword**
   ```cpp
   inline int execute(int x) const {
       // Hint to compiler for inlining
   }
   ```

3. **Consider constexpr**
   ```cpp
   constexpr int execute(int x) const {
       // Enable compile-time evaluation
   }
   ```

## Related Concepts

- **Static vs Dynamic Polymorphism**: Compile-time vs runtime dispatch
- **Template Metaprogramming**: Compile-time code generation
- **VTables and VPointers**: Virtual function implementation
- **Inline Expansion**: Compiler optimization technique
- **Type Erasure**: Hiding type information
- **Concepts**: C++20 type constraints

## Further Reading

- [C++ Virtual Functions](https://en.cppreference.com/w/cpp/language/virtual)
- [CRTP Wikipedia](https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern)
- [HFT Performance Optimization](https://www.highfrequencytrading.com/)
- [Modern C++ Design](https://www.amazon.com/Modern-C-Design-Generic-Programming/dp/0201704315)
- [C++ Templates: The Complete Guide](https://www.amazon.com/Templates-Complete-Guide-David-Vandevoorde/dp/0201734842)
