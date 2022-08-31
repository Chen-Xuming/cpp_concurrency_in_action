//
// Created by chen on 2022/8/22.
//

#include <iostream>
#include <thread>
#include <shared_mutex>
#include <chrono>
#include <vector>

std::mutex mtx_print;   // 防止输出乱序的锁

void reader(int id, int &data, std::shared_mutex &mtx){
    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::shared_lock<std::shared_mutex> s_lock(mtx);
        std::lock_guard<std::mutex> lk_print(mtx_print);
        std::cout << "[reader] #" << id << " reads data: " << data << std::endl;
    }
}

void writer(int id, int &data, std::shared_mutex &mtx){
    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::lock_guard<std::shared_mutex> s_lock(mtx);
        data++;
        std::lock_guard<std::mutex> lk_print(mtx_print);
        std::cout << "[writer] #" << id << " writes data: " << data << std::endl;
    }
}

int main(){
    int data = 1000;
    std::shared_mutex mtx;

    std::vector<std::thread> readers(5);
    for(int i = 0; i < 5; i++){
        readers[i] = std::thread(reader, i, std::ref(data), std::ref(mtx));
    }
    std::thread thread_writer(writer, 0, std::ref(data), std::ref(mtx));

    for(int i = 0; i < 5; i++){
        readers[i].join();
    }
    thread_writer.join();

    return 0;
}