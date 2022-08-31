//
// Created by doris on 2022/8/24.
//
#include <iostream>
#include <thread>
#include <future>

int sum(int a, int b){
    return a + b;
}

int main(){
    std::packaged_task<int(int, int)> task(sum);
    std::future<int> future = task.get_future();
    std::thread t(std::move(task), 10, 90);     // packaged_task 只可移动不可复制
//    task(10, 90);
    std::cout << "ans = " << future.get() << std::endl;
    t.join();
    return 0;
}