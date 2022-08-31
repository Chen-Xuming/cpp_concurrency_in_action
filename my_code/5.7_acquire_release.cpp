//
// Created by chen on 2022/8/30.
//

#include <atomic>
#include <thread>
#include <iostream>
#include <cassert>

std::atomic<bool> x, y;
std::atomic<int> z;

void write_x(){
    x.store(true, std::memory_order_release);
}

void write_y(){
    y.store(true, std::memory_order_release);
}

void read_x_then_y(){
    while(!x.load(std::memory_order_acquire)){}
    if(y.load(std::memory_order_acquire)){          // 即使write_y()先执行，此处仍有可能读取到旧值false
        ++z;
    }
}

void read_y_then_x(){
    while(!y.load(std::memory_order_acquire)){}
    if(x.load(std::memory_order_acquire)){          // 即使write_x()先执行，此处仍有可能读取到旧值false
        ++z;
    }
}

int main(){
    x = false;
    y = false;
    z = 0;
    std::thread t1(write_x), t2(write_y), t3(read_x_then_y), t4(read_y_then_x);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    std::cout << z << std::endl;
    assert(z.load() != 0);
    return 0;
}