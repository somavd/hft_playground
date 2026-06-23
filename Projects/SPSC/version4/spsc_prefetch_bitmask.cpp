#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <cassert>
#include <cstdint>
#include <atomic>

constexpr size_t ITERATIONS = 10'000'000;

struct Message {
    uint64_t sequence;
};


template<typename T>
class SPSCQueue {
private:
    static constexpr size_t SIZE = 16384;  // Power of 2
    std::vector<T> buffer_;
    alignas(64) std::atomic<size_t> head{0};
    alignas(64) std::atomic<size_t> tail{0};

public:
    SPSCQueue() {
        buffer_.resize(SIZE);
    }

    bool push(const T& item) {
        size_t current_tail = tail.load(std::memory_order_relaxed);
        size_t next_tail = (current_tail + 1) & (SIZE - 1);  // Bitmask
        if (next_tail == head.load(std::memory_order_acquire)) {
            return false; //Queue is full
        }
        buffer_[current_tail] = item;
        __builtin_prefetch(&buffer_[(current_tail + 2) & (SIZE - 1)], 0, 3);  // Prefetch
        tail.store(next_tail, std::memory_order_release);
        return true;
    }

    bool pop(T& item) {
        size_t current_head = head.load(std::memory_order_relaxed);
        if(current_head == tail.load(std::memory_order_acquire)) {
            return false; //Queue is empty
        }
        item = buffer_[current_head];
        __builtin_prefetch(&buffer_[(current_head + 2) & (SIZE - 1)], 0, 3);  // Prefetch
        head.store((current_head + 1) & (SIZE - 1), std::memory_order_release);  // Bitmask
        return true;
    }
};

class Timer {
public:
    static uint64_t now() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count();
    }
};

void Producer(SPSCQueue<Message>& queue) {
    Message msg;
    for(size_t i=0;i<ITERATIONS;i++) {
        msg.sequence = i;
        while(!queue.push(msg)) {
            // Backpressure: wait if queue is full
        }
    }
}

void Consumer(SPSCQueue<Message>& queue) {
    Message msg;
    for(size_t i=0;i<ITERATIONS;i++) {
        while(!queue.pop(msg)) {
            // Busy wait
        }
        assert(msg.sequence == i);
    }
}

int main() {
    SPSCQueue<Message> queue;
    auto start = Timer::now();
    std::thread producer_thread(Producer, std::ref(queue));
    std::thread consumer_thread(Consumer, std::ref(queue));
    
    producer_thread.join();
    consumer_thread.join();

    auto end = Timer::now();
    double seconds =(end-start)/1e9;
    double throughput = ITERATIONS / seconds;
    
    std::cout << "Time taken: " << end - start << " ns" << std::endl;
    std::cout << "Throughput: " << throughput << " msg/s" << std::endl;
    return 0;
}
