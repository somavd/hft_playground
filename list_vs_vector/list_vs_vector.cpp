#include <iostream>
#include <list>
#include <vector>
#include <chrono>

constexpr int ITERATIONS = 1'000'000;

int main() {
    std::list<int> list;
    std::vector<int> vector;
    
    for(int i=0;i<ITERATIONS;i++) {
        list.push_back(i);
        vector.push_back(i);
    }

    volatile long long sum = 0;

    auto start = std::chrono::high_resolution_clock::now();
    for(int x: list) {
        sum += x;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "List time: " << duration.count() << " microseconds" << std::endl;

    sum = 0;
    start = std::chrono::high_resolution_clock::now();
    for(int x: vector) {
        sum += x;
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Vector time: " << duration.count() << " microseconds" << std::endl;

    return 0;
}