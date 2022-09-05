//
// Created by chen on 2022/9/5.
//

#ifndef CPP_CONCURRENCY_IN_ACTION_THREADPOOL_H
#define CPP_CONCURRENCY_IN_ACTION_THREADPOOL_H

#include "work_stealing_queue.h"
#include "../6.7_threadsafe_queue_final.h"
#include <atomic>
#include <vector>
#include <future>
#include <iostream>

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
    thread_pool(): done(false), joiner(threads){
        size_t const thread_count = std::thread::hardware_concurrency();
        try{
            for(size_t i = 0; i < thread_count; i++){
                queues.push_back(std::unique_ptr<work_stealing_queue>(new work_stealing_queue));
            }
            for(size_t i = 0; i < thread_count; ++i){
                threads.push_back(std::thread(&thread_pool::work_thread, this, i));
            }
        }catch (...){
            done = true;
            throw;
        }
    }

    ~thread_pool(){
        done = true;
    }

    template<class Function>
    auto submit(Function f) -> std::future<typename std::result_of<Function()>::type>{
        using result_type = typename std::result_of<Function()>::type;
        std::packaged_task<result_type ()> task(f);
        std::future<result_type> res(task.get_future());
        if(local_work_queue){
            std::cout << "push to local queue." << std::endl;
            local_work_queue->push(std::move(task));
        }else{
            std::cout << "push to global queue." << std::endl;
            pool_work_queue.push(std::move(task));
        }
        return res;
    }

    void run_pending_task(){
        task_type task;
        if(pop_task_from_local_queue(task) || pop_task_from_pool_queue(task) || pop_task_from_other_thread_queue(task)){
            task();
        }else{
            std::this_thread::yield();
        }
    }

private:
    using task_type = function_wrapper;
    std::atomic<bool> done;
    threadsafe_queue<task_type> pool_work_queue;
    std::vector<std::unique_ptr<work_stealing_queue>> queues;
    std::vector<std::thread> threads;
    join_threads joiner;

    static thread_local work_stealing_queue *local_work_queue;
    static thread_local unsigned thread_index;

    void work_thread(unsigned index){
        thread_index = index;
        local_work_queue = queues[thread_index].get();
        while(!done){
            run_pending_task();
        }
    }

    bool pop_task_from_local_queue(task_type &task){
        return local_work_queue && local_work_queue->try_pop(task);
    }

    bool pop_task_from_pool_queue(task_type &task){
        return pool_work_queue.try_pop(task);
    }

    // 尝试从其它线程中窃取任务
    // 接着上次访问的任务队列继续访问，防止前几个线程频遭窃取
    bool pop_task_from_other_thread_queue(task_type &task){
        for(unsigned i = 0; i < queues.size(); ++i){
            unsigned const index = (thread_index + i + 1) % queues.size();
            if(queues[index]->try_steal(task)){
                return true;
            }
        }
        return false;
    }
};

thread_local work_stealing_queue* thread_pool::local_work_queue = nullptr;
thread_local unsigned thread_pool::thread_index = 0;

#endif //CPP_CONCURRENCY_IN_ACTION_THREADPOOL_H



















