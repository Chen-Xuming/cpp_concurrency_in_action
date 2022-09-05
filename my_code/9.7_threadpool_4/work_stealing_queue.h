//
// Created by chen on 2022/9/5.
//

// 基于deque双端队列的线程安全任务窃取队列
// 具体而言，持有该队列的线程从队头领取任务，其它线程则从队尾领取任务
// 这里为了简化实现，只采用了粗粒度的锁

#ifndef CPP_CONCURRENCY_IN_ACTION_WORK_STEALING_QUEUE_H
#define CPP_CONCURRENCY_IN_ACTION_WORK_STEALING_QUEUE_H

#include <deque>
#include "function_wrapper.h"

class work_stealing_queue{
private:
    using data_type = function_wrapper;
    std::deque<data_type> task_queue;
    mutable std::mutex mtx;
public:
    work_stealing_queue(){}
    work_stealing_queue(const work_stealing_queue &other) = delete;
    work_stealing_queue& operator=(const work_stealing_queue &other) = delete;

    void push(data_type data){
        std::lock_guard<std::mutex> lock(mtx);
        task_queue.push_front(std::move(data));
    }

    bool empty() const{
        std::lock_guard<std::mutex> lock(mtx);
        return task_queue.empty();
    }

    bool try_pop(data_type &res){
        std::lock_guard<std::mutex> lock(mtx);
        if(task_queue.empty()){
            return false;
        }
        res = std::move(task_queue.front());    // 本地线程从队头领取任务
        task_queue.pop_front();
        return true;
    }

    bool try_steal(data_type &res){
        std::lock_guard<std::mutex> lock(mtx);
        if(task_queue.empty()){
            return false;
        }
        res = std::move(task_queue.back());    // 本地线程从队头领取任务
        task_queue.pop_back();
        return true;
    }
};

#endif //CPP_CONCURRENCY_IN_ACTION_WORK_STEALING_QUEUE_H
