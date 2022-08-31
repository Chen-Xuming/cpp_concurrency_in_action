//
// Created by chen on 2022/8/30.
//

#include <atomic>
#include <cassert>
#include <thread>

std::atomic<bool> x = false;
std::atomic<bool> y = false;
std::atomic<int> v[2];

void f() {
    // v[0]、v[1] 的设置没有先后顺序，但都 happens-before 1
    v[0].store(1, std::memory_order_relaxed);
    v[1].store(2, std::memory_order_relaxed);
    x.store(true,std::memory_order_release);  // 1 happens-before 2（由于 2 的循环）
}

void g() {
    while (!x.load(std::memory_order_acquire)) {}  // 2：happens-before 3

    y.store(true,std::memory_order_release);  // 3 happens-before 4（由于 4 的循环）
}

void h() {
    while (!y.load(std::memory_order_acquire)) {} // 4 happens-before v[0]、v[1] 的读取
    assert(v[0].load(std::memory_order_relaxed) == 1);
    assert(v[1].load(std::memory_order_relaxed) == 2);
}

int main(){
    std::thread t1(f), t2(g), t3(h);
    t1.join();
    t2.join();
    t3.join();
}