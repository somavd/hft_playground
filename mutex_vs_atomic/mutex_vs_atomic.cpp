#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>

constexpr int NUM_THREADS = 2;
constexpr int ITERATIONS = 10'000'000;

// Mutex-based counter
int mutex_counter = 0;
std::mutex mtx;

// Atomic counter for comparison
std::atomic<int> atomic_counter{0};

void mutex_worker() {
    for (int i = 0; i < ITERATIONS; ++i) {
        std::lock_guard<std::mutex> lock(mtx);
        mutex_counter++;
    }
}

void atomic_worker() {
    for (int i = 0; i < ITERATIONS; ++i) {
        atomic_counter.fetch_add(1, std::memory_order_relaxed);
    }
}

auto getTime() {
    return std::chrono::high_resolution_clock::now();
}

int main() {
    std::cout << "=== Mutex vs Atomic Performance Comparison ===" << std::endl;
    std::cout << "Threads: " << NUM_THREADS << ", Iterations per thread: " << ITERATIONS << std::endl;
    std::cout << "Expected result: " << NUM_THREADS * ITERATIONS << "\n" << std::endl;

    // Test mutex performance
    mutex_counter = 0;
    auto start1 = getTime();
    std::thread t1(mutex_worker);
    std::thread t2(mutex_worker);
    t1.join();
    t2.join();
    auto end1 = getTime();
    auto mutex_time = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count();

    std::cout << "Mutex-based counter:" << std::endl;
    std::cout << "  Result: " << mutex_counter << " (Expected: " << NUM_THREADS * ITERATIONS << ")" << std::endl;
    std::cout << "  Time: " << mutex_time << " ms" << std::endl;
    std::cout << "  Status: " << (mutex_counter == NUM_THREADS * ITERATIONS ? "CORRECT" : "WRONG") << "\n" << std::endl;

    // Test atomic performance
    atomic_counter = 0;
    auto start2 = getTime();
    std::thread t3(atomic_worker);
    std::thread t4(atomic_worker);
    t3.join();
    t4.join();
    auto end2 = getTime();
    auto atomic_time = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2).count();

    std::cout << "Atomic counter:" << std::endl;
    std::cout << "  Result: " << atomic_counter.load() << " (Expected: " << NUM_THREADS * ITERATIONS << ")" << std::endl;
    std::cout << "  Time: " << atomic_time << " ms" << std::endl;
    std::cout << "  Status: " << (atomic_counter.load() == NUM_THREADS * ITERATIONS ? "CORRECT" : "WRONG") << "\n" << std::endl;

    // Calculate performance difference
    std::cout << "=== Summary ===" << std::endl;
    if (atomic_time > 0) {
        double speedup = (double)mutex_time / atomic_time;
        std::cout << "Atomic is " << speedup << "x faster than mutex" << std::endl;
    }
    std::cout << "Both provide correct results, but atomic is faster for simple operations" << std::endl;

    return 0;
}
