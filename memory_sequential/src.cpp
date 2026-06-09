#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

constexpr int NUM_THREADS = 2;
constexpr int ITERATIONS = 10000000;

std::atomic<int> seq_cst_counter{0};

void seq_cst_worker() {
    for (int i = 0; i < ITERATIONS; ++i) {
        seq_cst_counter.fetch_add(1, std::memory_order_seq_cst);
    }
}

int main() {
    std::cout << "Sequentially Consistent Memory Order Benchmark\n";
    std::cout << "===============================================\n";
    
    seq_cst_counter = 0;
    auto start = std::chrono::high_resolution_clock::now();
    std::thread t1(seq_cst_worker);
    std::thread t2(seq_cst_worker);
    t1.join();
    t2.join();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    std::cout << "Sequentially Consistent fetch_add - Time: " << duration << " ms\n";
    std::cout << "Result: " << seq_cst_counter.load() << " (Expected: " << NUM_THREADS * ITERATIONS << ")\n";
    
    std::cout << "\nMemory Order: Sequentially Consistent\n";
    std::cout << "- Strongest memory ordering guarantee\n";
    std::cout << "- All operations appear in single total order\n";
    std::cout << "- Default for atomic operations\n";
    std::cout << "- Easiest to reason about, but slower\n";
    
    return 0;
}
