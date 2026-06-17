#include <iostream>
#include <chrono>
#include <cassert>
#include <new>
#include <vector>

constexpr int ITERATIONS = 1'000'000;
volatile uint64_t sink = 0;

template<typename T>
class MemoryPool {
private:
    struct FreeNode {
        FreeNode *next;
    };

    FreeNode* free_list;
    T* pool;
    size_t pool_size;
public:
    MemoryPool(size_t size) : pool_size(size), free_list(nullptr) {
        pool = static_cast<T*>(::operator new(sizeof(T) * size));

        // Initialize free list in correct order
        for (size_t i = 0; i < pool_size; ++i) {
            FreeNode* node = reinterpret_cast<FreeNode*>(pool + i);
            node->next = free_list;
            free_list = node;
        }
    }

    ~MemoryPool() {
        ::operator delete(pool);
    }

    T* allocate() {
        if (free_list == nullptr) {
            return nullptr;
        }
        T* result = reinterpret_cast<T*>(free_list);
        free_list = free_list->next;
        return result;
    }

    void deallocate(T* ptr) {
        FreeNode* node = reinterpret_cast<FreeNode*>(ptr);
        node->next = free_list;
        free_list = node;
    }

};

struct Order {
    int id;
    double price;
    int size;

    Order(int id, double price, int size) : id(id), price(price), size(size) {}
};

void benchmark_new_delete() {
    auto start = std::chrono::high_resolution_clock::now();
    for(int i=0;i<ITERATIONS;i++) {
        Order* order = new Order(i, 100.0, 10);
        sink += order->id;
        delete order;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "New/Delete time: " << duration.count() << " microseconds" << std::endl;   
}

void benchmark_memory_pool() {
    MemoryPool<Order> pool(ITERATIONS);
    auto start = std::chrono::high_resolution_clock::now();
    for(int i=0;i<ITERATIONS;i++) {
        Order* memory = pool.allocate();
        assert(memory != nullptr);

        Order* order = new(memory) Order(i, 100.0, 10);
        sink += order->id;
        order->~Order();

        pool.deallocate(order);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Memory Pool time: " << duration.count() << " microseconds" << std::endl;
}

void benchmark_new_delete_vector() {
    std::vector<Order*> orders;
    orders.reserve(ITERATIONS);

    auto start = std::chrono::high_resolution_clock::now();
    for(int i=0; i<ITERATIONS; i++) {
        Order* order = new Order(i, 100.0, 10);
        orders.push_back(order);
    }

    for(auto* order : orders) {
        sink += order->id;
        delete order;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "New/Delete (vector) time: " << duration.count() << " microseconds" << std::endl;
}

void benchmark_memory_pool_vector() {
    std::vector<Order*> orders;
    orders.reserve(ITERATIONS);
    MemoryPool<Order> pool(ITERATIONS);

    auto start = std::chrono::high_resolution_clock::now();
    for(int i=0; i<ITERATIONS; i++) {
        auto* memory = pool.allocate();
        auto* order = new(memory) Order(i, 100.0, 10);
        orders.push_back(order);
    }

    for(auto* order : orders) {
        sink += order->id;
        order->~Order();
        pool.deallocate(order);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Memory Pool (vector) time: " << duration.count() << " microseconds" << std::endl;
}

int main() {
    std::cout << "=== Individual Allocation/Deallocation ===" << std::endl;
    benchmark_new_delete();
    benchmark_memory_pool();

    std::cout << "\n=== Vector-based Allocation/Deallocation ===" << std::endl;
    benchmark_new_delete_vector();
    benchmark_memory_pool_vector();

    return 0;
}