# Branch Prediction Demo

Demonstrates how CPU branch prediction affects performance by comparing sorted (predictable) vs unsorted (random) data with the same conditional logic.

## Files

- `branch_prediction.cpp` - Single-file demonstration
- `README.md` - This file

## Compile and Run

```bash
cd branch_prediction
g++ -O3 -std=c++20 branch_prediction.cpp -o branch_prediction
./branch_prediction
```

## Expected Results

```
=== Branch Prediction Demo ===
Same data, same work, different branch predictability

Unsorted (random branches):  XXXXX us
Sorted (predictable):       XXXXX us
Misprediction penalty:       X.XXx slower
```

## Explanation

The code performs the same operation (sum values > threshold) on the same data in two passes:

1. **Unsorted**: Random branch pattern → CPU branch predictor frequently mispredicts → pipeline flushes → slower
2. **Sorted**: Predictable branch pattern (first half false, second half true) → predictor learns pattern → faster

This demonstrates why HFT systems care about data layout and branch predictability.

## What is Branch Prediction?

Modern CPUs use a **pipeline** to execute instructions. When encountering an `if` statement, the CPU must guess (predict) which branch will be taken before the condition is evaluated. If the prediction is wrong, the pipeline must be flushed and restarted — this is called a **branch misprediction penalty** (~10-20 cycles).

### Predictable Branches (Sorted Data)

```
Sorted: [1, 3, 5, 12, 45, ... 130, 140, 200, 255]
         ← all FALSE →      ← all TRUE →
```

The branch predictor learns the pattern quickly:
- First half: always NOT taken → predictor learns "not taken"
- Second half: always taken → predictor learns "taken"
- Very few mispredictions

### Unpredictable Branches (Unsorted Data)

```
Unsorted: [200, 3, 180, 12, 45, 250, 5, 130, ...]
           T    F   T    F   F   T    F   T
```

The branch predictor cannot find a pattern:
- Random mix of taken/not-taken
- ~50% misprediction rate
- Each misprediction costs 10-20 cycles

## Issues with Original Benchmark

The original `branch_prediction.cpp` has a **flawed comparison**:

```cpp
// Predictable: threshold > 500000 → ~50% of values pass
if(predictableData[i] > 500000) { sum += predictableData[i]; }

// Random: threshold > 50 → ~99.99% of rand() values pass
if(randomData[i] > 50) { sum += randomData[i]; }
```

**Problems:**
1. Different thresholds mean different amounts of work
2. Random data does ~2x more additions than predictable
3. Measures arithmetic work, not branch prediction
4. Predictable data (0..999999) is actually 50/50 split at 500000

## Fixed Benchmark Design

The optimized version uses the **classic sorted vs unsorted** approach:

```cpp
// Same data, same threshold, same work
std::vector<int> data(ITERATIONS);
for(int i = 0; i < ITERATIONS; i++) {
    data[i] = std::rand() % 256;  // Values 0-255
}

// Unsorted: random branch pattern
benchmark_unsorted(data);

// Sorted: predictable branch pattern
std::sort(data.begin(), data.end());
benchmark_sorted(data);
```

**Why this works:**
- Same data values → same total work
- Same threshold (128) → same number of additions
- Only difference is branch predictability
- Sorted = predictable → fast
- Unsorted = random → slow

## Branchless Technique (HFT Optimization)

```cpp
// Branch version (branch misprediction possible)
if(data[i] >= THRESHOLD) {
    sum += data[i];
}

// Branchless version (no branch at all)
int mask = -(data[i] >= THRESHOLD);  // 0 or -1 (all bits set)
sum += (data[i] & mask);             // 0 or data[i]
```

**How it works:**
1. `data[i] >= THRESHOLD` evaluates to 0 or 1
2. Negating: `-(0)` = 0, `-(1)` = -1 (0xFFFFFFFF in two's complement)
3. `data[i] & 0` = 0 (value masked out)
4. `data[i] & 0xFFFFFFFF` = `data[i]` (value passes through)
5. No branch instruction → no misprediction possible

**Trade-off:**
- Always does the same work (mask + AND + add)
- Faster than mispredicted branches on random data
- May be slower than well-predicted branches on sorted data
- Consistent, predictable latency (important for HFT)

## HFT Applications

### Order Matching

```cpp
// Branch version (unpredictable if orders are random)
if (order.price >= best_ask) {
    execute_trade(order);
}

// Branchless version
int should_execute = -(order.price >= best_ask);
execute_count += (1 & should_execute);
```

### Signal Processing

```cpp
// Branch version
if (signal > threshold) {
    action = BUY;
} else {
    action = SELL;
}

// Branchless version
action = BUY * (signal > threshold) + SELL * (signal <= threshold);
```

### Key Takeaway

In HFT, **consistent latency** matters more than average throughput. Branchless code eliminates the worst-case branch misprediction penalty, providing more deterministic execution times.

## Further Reading

- [Why is processing a sorted array faster than processing an unsorted array?](https://stackoverflow.com/questions/11227809)
- [Branch Prediction - Agner Fog](https://www.agner.org/optimize/microarchitecture.pdf)
- [Branchless Programming Techniques](https://www.chessprogramming.org/Avoiding_Branches)
