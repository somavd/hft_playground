#include <iostream>
#include <atomic>
#include <chrono>
#include <thread>

constexpr int NUM_ITERATIONS = 100000000;

struct FalseSharing {
    std::atomic<long long> counter1 {0};
    std::atomic<long long> counter2 {0};
};

struct alignas(64) PaddedAtomic {
    std::atomic<long long> value {0};
};

struct NoFalseSharing {
    PaddedAtomic counter1;
    PaddedAtomic counter2;
};

void runFalseSharing() {
    FalseSharing data;
    auto start = std::chrono::high_resolution_clock::now();

    std::thread t1([&data]() {
        for (int i = 0; i < NUM_ITERATIONS; ++i) {
            data.counter1.fetch_add(1, std::memory_order_relaxed);
        }
    });

    std::thread t2([&data]() {
        for (int i = 0; i < NUM_ITERATIONS; ++i) {
            data.counter2.fetch_add(1, std::memory_order_relaxed);
        }
    });

    t1.join();
    t2.join();

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "False sharing time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;
}

void runNoFalseSharing() {
    NoFalseSharing data;
    auto start = std::chrono::high_resolution_clock::now();
    
    std::thread t1([&data]() {
        for (int i = 0; i < NUM_ITERATIONS; ++i) {
            data.counter1.value.fetch_add(1, std::memory_order_relaxed);
        }
    });
    
    std::thread t2([&data]() {
        for (int i = 0; i < NUM_ITERATIONS; ++i) {
            data.counter2.value.fetch_add(1, std::memory_order_relaxed);
        }
    });
    
    t1.join();
    t2.join();
    
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "No false sharing time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;
}

int main() {
    std::cout<< "Running Benchmark..." << std::endl;
    runFalseSharing();
    runNoFalseSharing();
    return 0;
}
