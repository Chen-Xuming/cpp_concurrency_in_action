//
// Created by chen on 2022/8/30.
//

#include <atomic>
#include <cassert>
#include <thread>

std::atomic<int> x = 0;
std::atomic<int> v[2];

void f() {
    v[0].store(1, std::memory_order_relaxed);
    v[1].store(2, std::memory_order_relaxed);
    x.store(1, std::memory_order_release);  // 1 happens-before 2（由于 2 的循环）
}

void g() {
    int i = 1;
    while (!x.compare_exchange_strong(
            i, 2,
            std::memory_order_acq_rel)) {  // 2 happens-before 3（由于 3 的循环）
        // x 为 1 时，将 x 替换为 2，返回 true
        // x 为 0 时，将 i 替换为 x，返回 false
        i = 1;  // 返回 false 时，x 未被替换，i 被替换为 0，因此将 i 重新设为 1
    }
}

void h() {
    while (x.load(std::memory_order_acquire) < 2) {  // 3
    }
    assert(v[0].load(std::memory_order_relaxed) == 1);
    assert(v[1].load(std::memory_order_relaxed) == 2);
}

int main(){
    std::thread t1(f), t2(g), t3(h);
    t1.join();
    t2.join();
    t3.join();
}