#include <iostream>
#include <vector>
#include <chrono>
#include <cstdlib>

constexpr int ITERATIONS = 1'000'000;

int main() {
    std::vector<int> predictableData(ITERATIONS);
    std::vector<int> randomData(ITERATIONS);
    volatile long long sum = 0;

    for(int i=0;i<ITERATIONS;i++) {
        predictableData[i] = i;
        randomData[i] = rand();
    }

    sum = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for(int i=0;i<ITERATIONS;i++) {
        if(predictableData[i] > 500000) {
            sum += predictableData[i];
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Time taken for predictable data: " << duration.count() << " microseconds" << std::endl;

    sum = 0;
    start = std::chrono::high_resolution_clock::now();
    for(int i=0;i<ITERATIONS;i++) {
        if(randomData[i] > 50) {
            sum += randomData[i];
        }
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Time taken for random data: " << duration.count() << " microseconds" << std::endl;
    
    return 0;
}