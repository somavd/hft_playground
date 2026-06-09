#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

constexpr int NUM_THREADS = 2;
constexpr int ITERATIONS = 10000000;

std::atomic<int> acq_rel_counter{0};

void acq_rel_worker() {
    for (int i = 0; i < ITERATIONS; ++i) {
        acq_rel_counter.fetch_add(1, std::memory_order_acq_rel);
    }
}

int main() {
    std::cout << "Acquire-Release Memory Order Benchmark\n";
    std::cout << "======================================\n";
    
    acq_rel_counter = 0;
    auto start = std::chrono::high_resolution_clock::now();
    std::thread t1(acq_rel_worker);
    std::thread t2(acq_rel_worker);
    t1.join();
    t2.join();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    std::cout << "Acquire-Release fetch_add - Time: " << duration << " ms\n";
    std::cout << "Result: " << acq_rel_counter.load() << " (Expected: " << NUM_THREADS * ITERATIONS << ")\n";
    
    std::cout << "\nMemory Order: Acquire-Release\n";
    std::cout << "- Synchronizes with release operations\n";
    std::cout << "- Prevents reordering of atomic operations\n";
    std::cout << "- Balanced performance and safety\n";
    std::cout << "- Use for synchronization between threads\n";
    
    return 0;
}
