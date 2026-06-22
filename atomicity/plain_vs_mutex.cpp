#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

constexpr int NUM_THREADS = 8;  // More threads = more contention
constexpr int ITERATIONS = 1'000'000;

// Plain counter - no synchronization (data race!)
int plain_counter = 0;

// Mutex-protected counter
int mutex_counter = 0;
std::mutex mtx;

void plain_worker() {
    for (int i = 0; i < ITERATIONS; ++i) {
        int temp = plain_counter;  // Read
        temp++;                    // Modify
        plain_counter = temp;      // Write (expanded to increase race window)
    }
}

void mutex_worker() {
    for (int i = 0; i < ITERATIONS; ++i) {
        std::lock_guard<std::mutex> lock(mtx);
        int temp = mutex_counter;
        temp++;
        mutex_counter = temp;
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
    std::thread t3(plain_worker);
    std::thread t4(plain_worker);
    std::thread t5(plain_worker);
    std::thread t6(plain_worker);
    std::thread t7(plain_worker);
    std::thread t8(plain_worker);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();
    t7.join();
    t8.join();
    auto end1 = getTime();
    auto plain_time = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count();

    std::cout << "Plain Counter (no synchronization):" << std::endl;
    std::cout << "  Result: " << plain_counter << " (Expected: " << NUM_THREADS * ITERATIONS << ")" << std::endl;
    std::cout << "  Time: " << plain_time << " ms" << std::endl;
    std::cout << "  Status: " << (plain_counter == NUM_THREADS * ITERATIONS ? "CORRECT" : "WRONG - Data race!") << "\n" << std::endl;

    // Test mutex counter (synchronized)
    mutex_counter = 0;
    auto start2 = getTime();
    std::thread t9(mutex_worker);
    std::thread t10(mutex_worker);
    std::thread t11(mutex_worker);
    std::thread t12(mutex_worker);
    std::thread t13(mutex_worker);
    std::thread t14(mutex_worker);
    std::thread t15(mutex_worker);
    std::thread t16(mutex_worker);
    t9.join();
    t10.join();
    t11.join();
    t12.join();
    t13.join();
    t14.join();
    t15.join();
    t16.join();
    auto end2 = getTime();
    auto mutex_time = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2).count();

    std::cout << "Mutex Counter (synchronized):" << std::endl;
    std::cout << "  Result: " << mutex_counter << " (Expected: " << NUM_THREADS * ITERATIONS << ")" << std::endl;
    std::cout << "  Time: " << mutex_time << " ms" << std::endl;
    std::cout << "  Status: " << (mutex_counter == NUM_THREADS * ITERATIONS ? "CORRECT" : "WRONG") << "\n" << std::endl;

    return 0;
}
