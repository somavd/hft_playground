#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

constexpr int NUM_THREADS = 2;
constexpr int ITERATIONS = 10000000;

std::atomic<int> relaxed_counter{0};

void relaxed_worker() {
    for (int i = 0; i < ITERATIONS; ++i) {
        relaxed_counter.fetch_add(1, std::memory_order_relaxed);
    }
}

int main() {
    std::cout << "Relaxed Memory Order Benchmark\n";
    std::cout << "================================\n";
    
    relaxed_counter = 0;
    auto start = std::chrono::high_resolution_clock::now();
    std::thread t1(relaxed_worker);
    std::thread t2(relaxed_worker);
    t1.join();
    t2.join();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    std::cout << "Relaxed fetch_add - Time: " << duration << " ms\n";
    std::cout << "Result: " << relaxed_counter.load() << " (Expected: " << NUM_THREADS * ITERATIONS << ")\n";
    
    std::cout << "\nMemory Order: Relaxed\n";
    std::cout << "- No synchronization guarantees\n";
    std::cout << "- Only atomicity is guaranteed\n";
    std::cout << "- Fastest atomic operation\n";
    std::cout << "- Use when no ordering is required\n";
    
    return 0;
}
