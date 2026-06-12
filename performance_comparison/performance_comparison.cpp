#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>

constexpr int NUM_THREADS = 2;
constexpr int ITERATIONS = 10000000;

// Timer utility for performance measurement
class Timer {
public:
    Timer() : start_time_(std::chrono::high_resolution_clock::now()) {}
    
    void reset() {
        start_time_ = std::chrono::high_resolution_clock::now();
    }
    
    long long elapsed_ms() const {
        auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time_).count();
    }

private:
    std::chrono::high_resolution_clock::time_point start_time_;
};

// --------------------------------------------------
// Test 1 : Plain int vs Atomic Counter
// --------------------------------------------------

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

void compare_normal_vs_atomic() {
    std::cout << "\n===== Normal vs Atomic Counter Comparison =====\n";
    
    // Test plain int (data race)
    plain_counter = 0;
    Timer timer1;
    std::thread t1(plain_worker);
    std::thread t2(plain_worker);
    t1.join();
    t2.join();
    auto plain_time = timer1.elapsed_ms();
    
    std::cout << "Plain int - Time: " << plain_time << " ms, Result: " << plain_counter << " (Expected: " << NUM_THREADS * ITERATIONS << ")\n";
    
    // Test atomic
    atomic_counter = 0;
    Timer timer2;
    std::thread t3(atomic_worker);
    std::thread t4(atomic_worker);
    t3.join();
    t4.join();
    auto atomic_time = timer2.elapsed_ms();
    
    std::cout << "Atomic    - Time: " << atomic_time << " ms, Result: " << atomic_counter.load() << "\n";
    std::cout << "Atomic is " << (plain_time > 0 ? (double)atomic_time / plain_time : 0) << "x slower than plain\n";
}

// --------------------------------------------------
// Test 2 : Fetch Add with Different Memory Models
// --------------------------------------------------

std::atomic<int> fetch_add_relaxed{0};
std::atomic<int> fetch_add_acq_rel{0};
std::atomic<int> fetch_add_seq_cst{0};

void fetch_add_relaxed_worker() {
    for (int i = 0; i < ITERATIONS; ++i) {
        fetch_add_relaxed.fetch_add(1, std::memory_order_relaxed);
    }
}

void fetch_add_acq_rel_worker() {
    for (int i = 0; i < ITERATIONS; ++i) {
        fetch_add_acq_rel.fetch_add(1, std::memory_order_acq_rel);
    }
}

void fetch_add_seq_cst_worker() {
    for (int i = 0; i < ITERATIONS; ++i) {
        fetch_add_seq_cst.fetch_add(1, std::memory_order_seq_cst);
    }
}

void compare_fetch_add_memory_models() {
    std::cout << "\n===== Fetch Add Memory Models Comparison =====\n";
    
    // Test relaxed
    fetch_add_relaxed = 0;
    Timer timer1;
    std::thread t1(fetch_add_relaxed_worker);
    std::thread t2(fetch_add_relaxed_worker);
    t1.join();
    t2.join();
    auto relaxed_time = timer1.elapsed_ms();
    
    std::cout << "Relaxed          - Time: " << relaxed_time << " ms, Result: " << fetch_add_relaxed.load() << "\n";
    
    // Test acquire-release
    fetch_add_acq_rel = 0;
    Timer timer2;
    std::thread t3(fetch_add_acq_rel_worker);
    std::thread t4(fetch_add_acq_rel_worker);
    t3.join();
    t4.join();
    auto acq_rel_time = timer2.elapsed_ms();
    
    std::cout << "Acquire-Release  - Time: " << acq_rel_time << " ms, Result: " << fetch_add_acq_rel.load() << "\n";
    
    // Test sequentially consistent
    fetch_add_seq_cst = 0;
    Timer timer3;
    std::thread t5(fetch_add_seq_cst_worker);
    std::thread t6(fetch_add_seq_cst_worker);
    t5.join();
    t6.join();
    auto seq_cst_time = timer3.elapsed_ms();
    
    std::cout << "Sequentially Cst - Time: " << seq_cst_time << " ms, Result: " << fetch_add_seq_cst.load() << "\n";
    
    std::cout << "Fastest: ";
    if (relaxed_time <= acq_rel_time && relaxed_time <= seq_cst_time) {
        std::cout << "Relaxed";
    } else if (acq_rel_time <= seq_cst_time) {
        std::cout << "Acquire-Release";
    } else {
        std::cout << "Sequentially Consistent";
    }
    std::cout << "\n";
}

// --------------------------------------------------
// Test 3 : Mutex vs Atomic Comparison
// --------------------------------------------------

int mutex_counter = 0;
std::mutex mtx;

void mutex_worker() {
    for (int i = 0; i < ITERATIONS; ++i) {
        std::lock_guard<std::mutex> lock(mtx);
        mutex_counter++;
    }
}

void compare_mutex_vs_atomic() {
    std::cout << "\n===== Mutex vs Atomic Comparison =====\n";
    
    // Test mutex
    mutex_counter = 0;
    Timer timer1;
    std::thread t1(mutex_worker);
    std::thread t2(mutex_worker);
    t1.join();
    t2.join();
    auto mutex_time = timer1.elapsed_ms();
    
    std::cout << "Mutex - Time: " << mutex_time << " ms, Result: " << mutex_counter << "\n";
    
    // Test atomic for comparison
    atomic_counter = 0;
    Timer timer2;
    std::thread t3(atomic_worker);
    std::thread t4(atomic_worker);
    t3.join();
    t4.join();
    auto atomic_time = timer2.elapsed_ms();
    
    std::cout << "Atomic - Time: " << atomic_time << " ms, Result: " << atomic_counter.load() << "\n";
    std::cout << "Atomic is " << (double)mutex_time / atomic_time << "x faster than mutex\n";
}

// --------------------------------------------------
// Main function to run all comparisons
// --------------------------------------------------

int main() {
    std::cout << "HFT Playground - Performance Comparison Suite\n";
    std::cout << "==============================================\n";
    
    compare_normal_vs_atomic();
    compare_fetch_add_memory_models();
    compare_mutex_vs_atomic();
    
    std::cout << "\nPerformance comparison completed!\n";
    return 0;
}
