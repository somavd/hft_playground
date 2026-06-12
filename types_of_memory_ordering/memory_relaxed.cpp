#include <iostream>
#include <atomic>
#include <thread>
#include <vector>

// Demonstrate memory_order_relaxed
std::atomic<int> counter{0};
std::atomic<int> data1{0};
std::atomic<int> data2{0};

void relaxed_counter_worker() {
    for (int i = 0; i < 1000000; ++i) {
        counter.fetch_add(1, std::memory_order_relaxed);
    }
}

void demonstrate_relaxed_behavior() {
    std::cout << "=== Demonstrating Relaxed Memory Order ===\n";
    
    // Reset counters
    counter.store(0, std::memory_order_relaxed);
    data1.store(0, std::memory_order_relaxed);
    data2.store(0, std::memory_order_relaxed);
    
    std::cout << "Starting relaxed counter increment with 4 threads...\n";
    
    std::vector<std::thread> threads;
    
    // Create 4 threads that increment counter
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(relaxed_counter_worker);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    int final_count = counter.load(std::memory_order_relaxed);
    std::cout << "Final counter value: " << final_count << "\n";
    std::cout << "Expected: 4000000\n";
    
    if (final_count == 4000000) {
        std::cout << "SUCCESS: Relaxed operations maintained atomicity!\n";
    } else {
        std::cout << "FAILURE: Lost some increments!\n";
    }
}

void demonstrate_no_ordering_guarantee() {
    std::cout << "\n=== Demonstrating No Ordering Guarantee ===\n";
    
    std::atomic<bool> stop_flag{false};
    std::vector<int> observed_values;
    
    // Thread 1: Write values in sequence
    std::thread writer([&] {
        data1.store(42, std::memory_order_relaxed);
        data2.store(100, std::memory_order_relaxed);
        stop_flag.store(true, std::memory_order_relaxed);
        std::cout << "Writer: Set data1=42, data2=100, stop_flag=true\n";
    });
    
    // Thread 2: Read values (may see reordering)
    std::thread reader([&] {
        while (!stop_flag.load(std::memory_order_relaxed)) {
            // Spin wait
        }
        
        // These reads might see reordered values
        int val1 = data1.load(std::memory_order_relaxed);
        int val2 = data2.load(std::memory_order_relaxed);
        
        std::cout << "Reader: Read data1=" << val1 << ", data2=" << val2 << "\n";
        
        if (val1 == 42 && val2 == 100) {
            std::cout << "Reader: Saw expected values (but not guaranteed!)\n";
        } else {
            std::cout << "Reader: Saw unexpected values due to relaxed ordering!\n";
            std::cout << "Reader: This demonstrates no ordering guarantee\n";
        }
    });
    
    writer.join();
    reader.join();
}

int main() {
    std::cout << "=== Memory Order Relaxed Demonstration ===\n\n";
    
    std::cout << "This demonstrates relaxed memory ordering:\n";
    std::cout << "- Only atomicity is guaranteed\n";
    std::cout << "- No ordering guarantees between operations\n";
    std::cout << "- Fastest but requires careful use\n\n";
    
    demonstrate_relaxed_behavior();
    demonstrate_no_ordering_guarantee();
    return 0;
}
