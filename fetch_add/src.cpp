#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>

constexpr int ITERATIONS = 1000000;

std::atomic<int> counter{0};

void worker() {
	for(int i = 0; i < ITERATIONS; i++) {
		counter.fetch_add(1, std::memory_order_relaxed);
	}
}

int main() {

	auto start = std::chrono::high_resolution_clock::now();
	std::thread t1(worker);
	std::thread t2(worker);

	t1.join();
	t2.join();
	auto end = std::chrono::high_resolution_clock::now();

	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end-start);

	std::cout<<"Time took: "<<duration<<"\n";
}

