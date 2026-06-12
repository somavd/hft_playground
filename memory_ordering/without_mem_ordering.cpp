#include<iostream>
#include<atomic>
#include<thread>

int data = 0;
std::atomic<bool> ready(false);

void producer() {
    data = 42;
    ready.store(true);
}

void consumer() {
    while (!ready.load());
    std::cout << "Data: " << data << std::endl;
}

int main() {
    std::thread t1(producer);
    std::thread t2(consumer);
    t1.join();
    t2.join();
    return 0;
}

