//
// Created by chen on 2022/9/4.
//

#include "6.7_threadsafe_queue_final.h"

#include <iostream>
#include <future>
#include <thread>
#include <memory>
#include <vector>


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

// 函数封装器
class function_wrapper{
public:
    template<class Function>
    function_wrapper(Function &&f): impl(new impl_type<Function>(std::move(f))){}
    function_wrapper() = default;
    function_wrapper(function_wrapper &&other): impl(std::move(other.impl)){}
    function_wrapper& operator=(function_wrapper &&other){
        impl = std::move(other.impl);
        return *this;
    }
    function_wrapper(const function_wrapper&) = delete;
    function_wrapper(function_wrapper&) = delete;
    function_wrapper& operator=(const function_wrapper&) = delete;

    void operator()(){
        impl->call();
    }

private:
    struct impl_base{
        virtual void call() = 0;
        virtual ~impl_base(){}
    };

    std::unique_ptr<impl_base> impl;

    template<class Function>
    struct impl_type: impl_base{
        Function f;
        explicit impl_type(Function &&f_): f(std::move(f_)){}
        void call(){
            f();
        }
    };
};

class thread_pool{
private:
    std::atomic_bool done;
    threadsafe_queue<function_wrapper> work_queue;
    std::vector<std::thread> threads;
    join_threads joiner;

    void worker_thread(){
        while (!done){
            function_wrapper task;
            if(work_queue.try_pop(task)){
                task();
            }else{
                std::this_thread::yield();
            }
        }
    }

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

    template <class Function>
    std::future<typename std::result_of<Function()>::type> submit(Function f) {
        typedef typename std::result_of<Function()>::type result_type;

        /*
         *   task是一个有返回值无参的packaged_task
         */
        std::packaged_task<result_type()> task(std::move(f));

        std::future<result_type> res(task.get_future());
        work_queue.push(std::move(task));
        return res;
    }
};

int main(){
    thread_pool tp;
    std::vector<std::future<int>> futures(100);
    for(int i = 0 ; i < 100; i++){
        futures[i] = tp.submit([i](){
            return i * i;
        });
    }

    for (int i = 0; i < 100; ++i) {
        std::cout << futures[i].get() << std::endl;
    }

    return 0;
}