#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <cassert>
#include <cstdint>

constexpr size_t ITERATIONS = 10'000'000;

struct Message {
    uint64_t sequence;
};

template<typename T>
class SPSCQueue {
private:
    std::vector<T> buffer_;
    size_t head = 0;
    size_t tail = 0;
    std::mutex mutex_;
    size_t max_size_;

public:
    SPSCQueue(size_t max_size = 10000) : max_size_(max_size) {
        buffer_.resize(max_size);
    }

    bool push(const T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t next_tail = (tail + 1) % max_size_;
        if (next_tail == head) {
            return false; // Queue is full
        }
        buffer_[tail] = item;
        tail = next_tail;
        return true;
    }

    bool pop(T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (head == tail) {
            return false; // Queue is empty
        }
        item = buffer_[head];
        head = (head + 1) % max_size_;
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
    SPSCQueue<Message> queue(16384);  // Max queue size of 16,384

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