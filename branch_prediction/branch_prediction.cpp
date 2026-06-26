#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <random>

constexpr int SIZE = 1'000'000;
constexpr int THRESHOLD = 500'000;

volatile long long sink = 0;

int main() {
    std::cout << "=== Branch Prediction Demo ===\n";
    std::cout << "Same data, same work, different branch predictability\n\n";

    // Generate random data
    std::mt19937 gen(42);
    std::uniform_int_distribution<int> dist(0, SIZE);
    std::vector<int> data(SIZE);
    for (int i = 0; i < SIZE; ++i) {
        data[i] = dist(gen);
    }

    // Test 1: Unsorted data (random branch pattern)
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < SIZE; ++i) {
        if (data[i] > THRESHOLD) {
            sink += data[i];
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto unsorted_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Unsorted (random branches):  " << unsorted_time.count() << " us\n";

    // Test 2: Sorted data (predictable branch pattern)
    std::sort(data.begin(), data.end());
    sink = 0;
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < SIZE; ++i) {
        if (data[i] > THRESHOLD) {
            sink += data[i];
        }
    }
    end = std::chrono::high_resolution_clock::now();
    auto sorted_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Sorted (predictable):       " << sorted_time.count() << " us\n";

    if (sorted_time.count() > 0) {
        std::cout << "Misprediction penalty:       " 
                  << (double)unsorted_time.count() / sorted_time.count() << "x slower\n";
    }

    return 0;
}