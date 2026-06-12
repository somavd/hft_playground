#include <iostream>
#include <atomic>
#include <thread>
#include <vector>

// Demonstrate memory_order_seq_cst (sequentially consistent)
std::atomic<int> data1{0};
std::atomic<int> data2{0};
std::atomic<bool> ready{false};

void producer() {
    // Write data with sequential consistency
    data1.store(42, std::memory_order_seq_cst);
    data2.store(100, std::memory_order_seq_cst);
    
    // Set ready flag with sequential consistency
    ready.store(true, std::memory_order_seq_cst);
    
    std::cout << "Producer: Set data1=42, data2=100, ready=true (seq_cst)\n";
}

void consumer() {
    // Wait for ready with sequential consistency
    while (!ready.load(std::memory_order_seq_cst)) {
        // Spin wait
    }
    
    // Read data with sequential consistency
    int val1 = data1.load(std::memory_order_seq_cst);
    int val2 = data2.load(std::memory_order_seq_cst);
    
    std::cout << "Consumer: Read data1=" << val1 << ", data2=" << val2 << " (seq_cst)\n";
    std::cout << "Consumer: Expected data1=42, data2=100\n";
    
    if (val1 == 42 && val2 == 100) {
        std::cout << "SUCCESS: Sequential consistency worked!\n";
    } else {
        std::cout << "FAILURE: Sequential consistency failed!\n";
    }
}

void demonstrate_global_order() {
    std::cout << "\n=== Demonstrating Global Total Order ===\n";
    
    std::atomic<int> counter{0};
    std::vector<std::thread> threads;
    std::vector<int> results(4);
    
    // Create 4 threads that increment and store values
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&, i] {
            int value = counter.fetch_add(1, std::memory_order_seq_cst) + 1;
            results[i] = value;
            std::cout << "Thread " << i << " got value: " << value << "\n";
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "Final counter value: " << counter.load() << "\n";
    std::cout << "All threads saw unique values due to sequential consistency\n";
}

int main() {
    std::cout << "=== Memory Order Sequentially Consistent Demonstration ===\n\n";
    
    std::cout << "This demonstrates the strongest memory ordering guarantee.\n";
    std::cout << "All operations appear in a single total order.\n\n";
    
    std::thread t1(producer);
    std::thread t2(consumer);
    
    t1.join();
    t2.join();
    
    demonstrate_global_order();
    
    return 0;
}
