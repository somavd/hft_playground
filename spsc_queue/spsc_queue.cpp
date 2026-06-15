#include <iostream>
#include <atomic>
#include <array>
#include <thread>
#include <unistd.h>

template<typename T, size_t SIZE>
class SPSCQueue {
private:
    std::array<T, SIZE> buff;
    std::atomic<int> head {0};
    std::atomic<int> tail {0};
public:
    bool push(const T &item) {
        int current_tail = tail.load(std::memory_order_relaxed);
        int next_tail = (current_tail + 1) % SIZE;
        if (next_tail == head.load(std::memory_order_acquire)) {
            std::cout<<"Queue is full"<<std::endl;
            return false; // Queue is full
        }
        buff[current_tail] = item;
        tail.store(next_tail, std::memory_order_release);
        return true;
    }

    bool pop(T &item) {
        int current_head = head.load(std::memory_order_relaxed);
        if (current_head == tail.load(std::memory_order_acquire)) {
            std::cout<<"Queue is empty"<<std::endl;
            return false; // Queue is empty
        }
        item = buff[current_head];
        head.store((current_head + 1) % SIZE, std::memory_order_release);
        return true;
    }
};


struct Tick {
    int price;
};
SPSCQueue<Tick, 1024> queue;

void producer() {
    for (int i = 0; i < 100; i++) {
        Tick tick;
        tick.price = i;
        while(!queue.push(tick)) {
            // Spin
        }
    }
}

void consumer() {
    for (int i = 0; i < 100; i++) {
        Tick tick;
        while(!queue.pop(tick)) {
            // Spin
        }
        std::cout << tick.price << std::endl;
    }
}

int main() {
    std::thread producer_thread(producer);
    std::thread consumer_thread(consumer);
    producer_thread.join();
    consumer_thread.join();
    return 0;
}