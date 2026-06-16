#include <iostream>
#include <chrono>

constexpr int ITERATIONS = 100'000'000;
class VirtualBase {
public:
    virtual int execute(int x) const {
        return x + 1;
    }
};

class VirtualDerived : public VirtualBase {
public:
    int execute(int x) const override {
        return x + 2;
    }
};

template<typename Derived>
class CRTPBase {
public:
    int execute(int x) const {
        return static_cast<const Derived*>(this)->executeImpl(x);
    }
};

class CRTPDerived : public CRTPBase<CRTPDerived> {
public:
    int executeImpl(int x) const {
        return x + 3;
    }
};

int main() {
    VirtualDerived virtual_derived;
    VirtualBase* basePtr = &virtual_derived;

    auto start1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERATIONS; ++i) {
        basePtr->execute(i);
    }
    auto end1 = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(end1 - start1);
    std::cout << "Virtual execution time: " << duration1.count() << " microseconds" << std::endl;

    CRTPDerived crtp_derived;
    
    auto start2 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERATIONS; ++i) {
        crtp_derived.execute(i);
    }
    auto end2 = std::chrono::high_resolution_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(end2 - start2);
    std::cout << "CRTP execution time: " << duration2.count() << " microseconds" << std::endl;
    
    return 0;
}
