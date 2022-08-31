//
// Created by chen on 2022/8/27.
//

#include <atomic>
#include <thread>
#include <iostream>
#include <vector>

class spinlock{
private:
    // true: 锁被持有；false: 锁未被持有
    std::atomic_flag flag;
public:
    spinlock(): flag(ATOMIC_FLAG_INIT){}    // 必须使用ATOMIC_FLAG_INIT初始化，置为false

    void lock(){
        // 当 flag = false 时终止等待，并置为 true
        while(flag.test_and_set(std::memory_order_acquire)){}
    }

    void unlock(){
        flag.clear(std::memory_order_release);
    }
};

spinlock m;

void f(int n) {
    for (int i = 0; i < 10; ++i) {
        m.lock();
        std::cout << "Output from thread " << n << '\n';
        m.unlock();
    }
}

int main() {
    std::vector<std::thread> v;
    for (int i = 0; i < 10; ++i) {
        v.emplace_back(f, i);
    }
    for (auto& x : v) {
        x.join();
    }
}