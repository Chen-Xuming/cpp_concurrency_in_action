//
// Created by chen on 2022/8/30.
//
#include <atomic>
#include <cassert>
#include <thread>

std::atomic<int*> x;
int i;

void producer() {
    int* p = new int(42);
    i = 42;
    x.store(p, std::memory_order_release);      // 前面两行不会重排到本行后面，所以两个assert成立
}

void consumer() {
    int* q;
    while (!(q = x.load(std::memory_order_acquire))) {
    }
    assert(*q == 42);  // 一定不出错
    assert(i == 42);   // 一定不出错
}

int main() {
    std::thread t1(producer);
    std::thread t2(consumer);
    t1.join();
    t2.join();
}