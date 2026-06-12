#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>

constexpr int NUM_THREADS = 2;
constexpr int ITERATIONS = 10000000;

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
        atomic_counter++;
    }
}

int main() {
    std::cout << "Mutex vs Atomic Comparison\n";
    std::cout << "==========================\n";
    
    // Test mutex performance
    mutex_counter = 0;
    auto start1 = std::chrono::high_resolution_clock::now();
    std::thread t1(mutex_worker);
    std::thread t2(mutex_worker);
    t1.join();
    t2.join();
    auto end1 = std::chrono::high_resolution_clock::now();
    auto mutex_time = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count();
    
    std::cout << "Mutex - Time: " << mutex_time << " ms, Result: " << mutex_counter << "\n";
    
    // Test atomic performance for comparison
    atomic_counter = 0;
    auto start2 = std::chrono::high_resolution_clock::now();
    std::thread t3(atomic_worker);
    std::thread t4(atomic_worker);
    t3.join();
    t4.join();
    auto end2 = std::chrono::high_resolution_clock::now();
    auto atomic_time = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2).count();
    
    std::cout << "Atomic - Time: " << atomic_time << " ms, Result: " << atomic_counter.load() << "\n";
    
    // Calculate performance difference
    if (atomic_time > 0) {
        double speedup = (double)mutex_time / atomic_time;
        std::cout << "Atomic is " << speedup << "x faster than mutex\n";
    }
    
    return 0;
}
