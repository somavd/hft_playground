#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

constexpr int NUM_THREADS = 2;
constexpr int ITERATIONS = 10'000'000;

// Plain counter - no synchronization (data race!)
int plain_counter = 0;

// Mutex-protected counter
int mutex_counter = 0;
std::mutex mtx;

void plain_worker() {
    for (int i = 0; i < ITERATIONS; ++i) {
        plain_counter++;  // Data race: read-modify-write not atomic
    }
}

void mutex_worker() {
    for (int i = 0; i < ITERATIONS; ++i) {
        std::lock_guard<std::mutex> lock(mtx);
        mutex_counter++;  // Protected by mutex
    }
}

auto getTime() {
    return std::chrono::high_resolution_clock::now();
}

int main() {
    std::cout << "=== Atomicity Demo: Plain vs Mutex ===" << std::endl;
    std::cout << "Threads: " << NUM_THREADS << ", Iterations per thread: " << ITERATIONS << std::endl;
    std::cout << "Expected result: " << NUM_THREADS * ITERATIONS << "\n" << std::endl;

    // Test plain counter (data race)
    plain_counter = 0;
    auto start1 = getTime();
    std::thread t1(plain_worker);
    std::thread t2(plain_worker);
    t1.join();
    t2.join();
    auto end1 = getTime();
    auto plain_time = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count();

    std::cout << "Plain Counter (no synchronization):" << std::endl;
    std::cout << "  Result: " << plain_counter << " (Expected: " << NUM_THREADS * ITERATIONS << ")" << std::endl;
    std::cout << "  Time: " << plain_time << " ms" << std::endl;
    std::cout << "  Status: " << (plain_counter == NUM_THREADS * ITERATIONS ? "CORRECT" : "WRONG - Data race!") << "\n" << std::endl;

    // Test mutex counter (synchronized)
    mutex_counter = 0;
    auto start2 = getTime();
    std::thread t3(mutex_worker);
    std::thread t4(mutex_worker);
    t3.join();
    t4.join();
    auto end2 = getTime();
    auto mutex_time = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2).count();

    std::cout << "Mutex Counter (synchronized):" << std::endl;
    std::cout << "  Result: " << mutex_counter << " (Expected: " << NUM_THREADS * ITERATIONS << ")" << std::endl;
    std::cout << "  Time: " << mutex_time << " ms" << std::endl;
    std::cout << "  Status: " << (mutex_counter == NUM_THREADS * ITERATIONS ? "CORRECT" : "WRONG") << "\n" << std::endl;

    return 0;
}
