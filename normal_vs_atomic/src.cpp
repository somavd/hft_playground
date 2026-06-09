#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

constexpr int NUM_THREADS = 2;
constexpr int ITERATIONS = 10000000;

int plain_counter = 0;
std::atomic<int> atomic_counter{0};

void plain_worker() {
    for (int i = 0; i < ITERATIONS; ++i) {
        plain_counter++;
    }
}

void atomic_worker() {
    for (int i = 0; i < ITERATIONS; ++i) {
        atomic_counter++;
    }
}

int main() {
    std::cout << "Normal vs Atomic Counter Comparison\n";
    std::cout << "====================================\n";
    
    // Test plain int (data race)
    plain_counter = 0;
    auto start1 = std::chrono::high_resolution_clock::now();
    std::thread t1(plain_worker);
    std::thread t2(plain_worker);
    t1.join();
    t2.join();
    auto end1 = std::chrono::high_resolution_clock::now();
    auto plain_time = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count();
    
    std::cout << "Plain int - Time: " << plain_time << " ms, Result: " << plain_counter;
    std::cout << " (Expected: " << NUM_THREADS * ITERATIONS << ")\n";
    
    // Test atomic
    atomic_counter = 0;
    auto start2 = std::chrono::high_resolution_clock::now();
    std::thread t3(atomic_worker);
    std::thread t4(atomic_worker);
    t3.join();
    t4.join();
    auto end2 = std::chrono::high_resolution_clock::now();
    auto atomic_time = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2).count();
    
    std::cout << "Atomic    - Time: " << atomic_time << " ms, Result: " << atomic_counter.load() << "\n";
    
    if (plain_time > 0) {
        double ratio = (double)atomic_time / plain_time;
        std::cout << "Atomic is " << ratio << "x slower than plain\n";
    }
    
    std::cout << "\nKey Insights:\n";
    std::cout << "- Plain int shows data race (incorrect results)\n";
    std::cout << "- Atomic provides thread safety with performance cost\n";
    std::cout << "- Atomic overhead depends on CPU architecture\n";
    
    return 0;
}
