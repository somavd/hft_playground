#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>

constexpr int NUM_THREADS = 2;
constexpr int ITERATIONS = 10000000;

// --------------------------------------------------
// Test 1 : Plain int (Data Race)
// --------------------------------------------------

int plain_counter = 0;

void plain_worker()
{
    for (int i = 0; i < ITERATIONS; ++i)
    {
        plain_counter++;
    }
}

void test_plain()
{
    plain_counter = 0;

    auto start = std::chrono::high_resolution_clock::now();

    std::thread t1(plain_worker);
    std::thread t2(plain_worker);

    t1.join();
    t2.join();

    auto end = std::chrono::high_resolution_clock::now();

    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start);

    std::cout << "===== Plain int =====\n";
    std::cout << "Expected : "
              << NUM_THREADS * ITERATIONS
              << "\n";

    std::cout << "Actual   : "
              << plain_counter
              << "\n";

    std::cout << "Time     : "
              << duration.count()
              << " ms\n\n";
}

// --------------------------------------------------
// Test 2 : Atomic
// --------------------------------------------------

std::atomic<int> atomic_counter{0};

void atomic_worker()
{
    for (int i = 0; i < ITERATIONS; ++i)
    {
        atomic_counter++;
    }
}

void test_atomic()
{
    atomic_counter = 0;

    auto start = std::chrono::high_resolution_clock::now();

    std::thread t1(atomic_worker);
    std::thread t2(atomic_worker);

    t1.join();
    t2.join();

    auto end = std::chrono::high_resolution_clock::now();

    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start);

    std::cout << "===== Atomic =====\n";
    std::cout << "Expected : "
              << NUM_THREADS * ITERATIONS
              << "\n";

    std::cout << "Actual   : "
              << atomic_counter.load()
              << "\n";

    std::cout << "Time     : "
              << duration.count()
              << " ms\n\n";
}

// --------------------------------------------------
// Test 3 : Mutex
// --------------------------------------------------

int mutex_counter = 0;
std::mutex mtx;

void mutex_worker()
{
    for (int i = 0; i < ITERATIONS; ++i)
    {
        std::lock_guard<std::mutex> lock(mtx);
        mutex_counter++;
    }
}

void test_mutex()
{
    mutex_counter = 0;

    auto start = std::chrono::high_resolution_clock::now();

    std::thread t1(mutex_worker);
    std::thread t2(mutex_worker);

    t1.join();
    t2.join();

    auto end = std::chrono::high_resolution_clock::now();

    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start);

    std::cout << "===== Mutex =====\n";
    std::cout << "Expected : "
              << NUM_THREADS * ITERATIONS
              << "\n";

    std::cout << "Actual   : "
              << mutex_counter
              << "\n";

    std::cout << "Time     : "
              << duration.count()
              << " ms\n\n";
}

// --------------------------------------------------

int main()
{
    test_plain();
    test_atomic();
    test_mutex();

    return 0;
}
