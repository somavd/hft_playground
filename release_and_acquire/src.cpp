#include<iostream>
#include<thread>
#include<chrono>

std::atomic<bool> ready(false);
int data = 0;

void producer() {
	data = 42;
	ready.store(true, std::memory_order_release);
}

void consumer() {
	while(!ready.load(std::memory_order_acquire));
	std::cout<<"Value of data : "<<data<<"\n";
}

int main() {
	std::thread t1(producer);
	std::thread t2(consumer);

	t1.join();
	t2.join();

}
