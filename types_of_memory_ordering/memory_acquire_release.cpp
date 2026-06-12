#include <iostream>
#include <atomic>
#include <thread>
#include <vector>

// Demonstrate memory_order_acquire and memory_order_release
std::atomic<int> data1{0};
std::atomic<int> data2{0};
std::atomic<bool> ready{false};

void producer() {
    // Write data first
    data1.store(42, std::memory_order_relaxed);
    data2.store(100, std::memory_order_relaxed);
    
    // Then release - ensures all previous writes are visible
    ready.store(true, std::memory_order_release);
    
    std::cout << "Producer: Set data1=42, data2=100, ready=true\n";
}

void consumer() {
    // Wait for ready with acquire - ensures all subsequent reads see latest writes
    while (!ready.load(std::memory_order_acquire)) {
        // Spin wait
    }
    
    // These reads are guaranteed to see the values written before release
    int val1 = data1.load(std::memory_order_relaxed);
    int val2 = data2.load(std::memory_order_relaxed);
    
    std::cout << "Consumer: Read data1=" << val1 << ", data2=" << val2 << "\n";
    std::cout << "Consumer: Expected data1=42, data2=100\n";
    
    if (val1 == 42 && val2 == 100) {
        std::cout << "SUCCESS: Acquire-Release synchronization worked!\n";
    } else {
        std::cout << "FAILURE: Synchronization failed!\n";
    }
}

int main() {
    std::cout << "=== Memory Order Acquire-Release Demonstration ===\n\n";
    
    std::cout << "This demonstrates how acquire-release semantics ensure\n";
    std::cout << "proper synchronization between producer and consumer.\n\n";
    
    std::thread t1(producer);
    std::thread t2(consumer);
    
    t1.join();
    t2.join();
    
    return 0;
}
