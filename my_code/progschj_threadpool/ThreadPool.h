//
// Created by chen on 2022/9/3.
//

// github上一个短小精悍的线程池实现
// https://github.com/progschj/ThreadPool

#ifndef CPP_CONCURRENCY_IN_ACTION_THREADPOOL_H
#define CPP_CONCURRENCY_IN_ACTION_THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <type_traits>

class ThreadPool{
public:
    ThreadPool(size_t);

    // std::result_of 可以获取返回类型
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;

    ~ThreadPool();
private:
    std::vector<std::thread> workers;       // 工作线程
    std::queue<std::function<void()>> tasks;  // 任务队列

    // 同步
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

// 在构造函数中将工作线程启动
inline ThreadPool::ThreadPool(size_t thread_num): stop(false) {
    for(size_t i = 0; i < thread_num; ++i){
        workers.emplace_back([this](){
            // 每个工作线程无线循环，不断接任务。
            for(;;){
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    condition.wait(lock, [this](){
                        return this->stop || !this->tasks.empty();
                    });
                    if(stop && tasks.empty())
                        return;
                    task = std::move(tasks.front());
                    tasks.pop();
                }// lock.unlock()

                task();
            }
        });
    }
}

// 向任务队列提交任务
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared< std::packaged_task<return_type()> >(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );

    std::future<return_type> res = task->get_future();

    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // stop=true 时不允许再提交任务（pool正在析构）
        if(stop){
            throw std::runtime_error("enqueue on stopped ThreadPool.");
        }

        /*
             std::queue<std::function<void()>> tasks
             向其添加一个用 void() 包装的函数：
             void wrapper(){
                // task是一个packaged_task指针
                // 可以带任意返回类型，任意参数
                (*task)();
             }
         */
        tasks.emplace([task](){
            (*task)();
        });
    }// lock.unlock();

    condition.notify_one();
    return res;
}

inline ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }

    // 1. stop = true
    // 2. 唤醒所有等待线程，催促它们把队列中的所有任务完成，随后所有线程结束
    condition.notify_all();

    for(auto &worker: workers){
        worker.join();
    }
}

#endif //CPP_CONCURRENCY_IN_ACTION_THREADPOOL_H



















