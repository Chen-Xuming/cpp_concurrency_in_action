//
// Created by chen on 2022/9/5.
//

#include "threadpool.h"
int main(){
    thread_pool tp;
    int task_count = 10;
    std::vector<std::future<int>> futures(task_count);

    std::this_thread::sleep_for(std::chrono::seconds(5));

    for(int i = 0 ; i < task_count; i++){
        futures[i] = tp.submit([i](){
            return i * i;
        });
    }

    for (int i = 0; i < task_count; ++i) {
        std::cout << futures[i].get() << std::endl;
    }

    return 0;
}