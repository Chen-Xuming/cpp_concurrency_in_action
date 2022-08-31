//
// Created by chen on 2022/8/30.
//

#include <atomic>
#include <thread>
#include <iostream>

std::atomic<int> x = 0;
std::atomic<int> y = 0;
int i, j;

void f() {
    i = y.load(std::memory_order_relaxed);  // 1
    x.store(i, std::memory_order_relaxed);      // 2
}

void g() {
    j = x.load(std::memory_order_relaxed);  // 3
    y.store(42, std::memory_order_relaxed);     // 4
}

int main() {
    std::thread t1(f);
    std::thread t2(g);
    t1.join();
    t2.join();
    // 可能执行顺序为 4123，结果 i == 42, j == 42

    std::cout << i << "  " << j << std::endl;
}