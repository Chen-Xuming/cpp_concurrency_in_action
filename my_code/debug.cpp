//
// Created by chen on 2022/9/4.
//
#include<iostream>
#include<thread>
#include<vector>
#include <mutex>
#include <memory>

class test{
private:
    std::vector<std::thread> threads;
//    static thread_local int data;
    static thread_local std::unique_ptr<int> data;
    std::mutex cout_mutex;

public:
    test(){
        int thread_count = 5;
        int thread_id = 100;
        for(int i = 0; i < 5; i++){
            threads.push_back(std::thread(&test::f, this, thread_id));
            thread_id++;
        }
    }

    void f(int id){
        if(data == nullptr) {
            std::cout << "data = nullptr" << std::endl;
            data = std::make_unique<int>(1);
        }
//        f2(id);
    }

    void f2(int id){
        for(int i = 0; i < 5; i++){
            std::cout << "[" << id << "] " << *data << std::endl;
            (*data) += 1;
        }
    }

    ~test(){
        for(auto &thr: threads){
            if(thr.joinable())
                thr.join();
        }
    }
};

thread_local std::unique_ptr<int> test::data = nullptr;

int main(){
    test t;
    for(int i = 0; i < 5; i++){
        t.f2(i);    // error
    }
    return 0;
}