//
// Created by chen on 2022/9/3.
//

// 一个最简单的线程池实现

#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include "6.7_threadsafe_queue_final.h"

// 利用RAII机制保证所有线程都能join
class join_threads{
private:
    std::vector<std::thread> &threads;
public:
    explicit join_threads(std::vector<std::thread> &threads_): threads(threads_){}
    ~join_threads(){
        std::cout << "~join_thread()." << std::endl;
        for(auto &t : threads){
            if(t.joinable()){
                t.join();
            }
        }
    }
};

class thread_pool{
public:
    // 在构造函数中启动所有工作线程
    thread_pool(): done(false), joiner(threads){
        size_t const thread_count = std::thread::hardware_concurrency();
        std::cout << "thread_count: " << thread_count << std::endl;
        try{
            for(size_t i = 0; i < thread_count; ++i){
                threads.emplace_back(std::thread(&thread_pool::worker_thread, this));
            }
        }catch (...){
            std::cout << "throw some exception." << std::endl;
            done = true;
            throw;          // ===> 通过joiner使所有线程在线程池销毁之前完成
        }
    }

    ~thread_pool(){
        std::cout << "~thread_pool()." << std::endl;
        done = true;
    }

    template<class Function>
    void submit(Function f){
        work_queue.push(std::function<void()>(f));
    }

private:
    /*
        成员变量声明的顺序是有讲究的，它们需要按照一定顺序析构：
        线程必须全部终结，任务队列才可以销毁。
     */
    std::atomic_bool done;
    threadsafe_queue<std::function<void()> > work_queue;
    std::vector<std::thread> threads;
    join_threads joiner;

    void worker_thread(){
        while(!done){
            std::function<void()> task;
            if(work_queue.try_pop(task)){
                task();
            } else{
                std::this_thread::yield();      // 如果没有获得任务，那么让出时间片剩余的时间
            }
        }
    }
};

int main(){
    thread_pool tp;
    for(int i = 0; i < 500; i++){
//        tp.submit([i](){
//            std::cout << i << " * " << i << " = " << i * i << std::endl;
//        });
        std::function<void()> f([i](){
            std::cout << "#" << i << " hello world" << std::endl;
        });
        tp.submit(f);
    }
    return 0;
}