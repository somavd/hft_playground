#include <iostream>
#include <list>
#include <vector>
#include <chrono>

constexpr int SIZE = 1'000'000;

volatile long long sink = 0;

int main() {
    std::cout << "=== List vs Vector Iteration (" << SIZE << " elements) ===\n";

    std::list<int> lst;
    std::vector<int> vec;
    vec.reserve(SIZE);

    for (int i = 0; i < SIZE; ++i) {
        lst.push_back(i);
        vec.push_back(i);
    }

    long long sum = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (int x : lst) {
        sum += x;
    }
    auto end = std::chrono::high_resolution_clock::now();
    sink = sum;
    auto list_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "List time:   " << list_time.count() << " us\n";

    sum = 0;
    start = std::chrono::high_resolution_clock::now();
    for (int x : vec) {
        sum += x;
    }
    end = std::chrono::high_resolution_clock::now();
    sink = sum;
    auto vec_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Vector time: " << vec_time.count() << " us\n";

    if (vec_time.count() > 0) {
        std::cout << "Vector is " << (double)list_time.count() / vec_time.count()
                  << "x faster (cache locality)\n";
    }

    return 0;
}