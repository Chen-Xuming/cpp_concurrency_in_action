//
// Created by chen on 2022/9/1.
//

#include "3.5_threadsafe_stack.h"
#include <iostream>
#include <atomic>
#include <future>
#include <thread>
#include <list>
#include <memory>
#include <vector>
#include <algorithm>
#include <random>

std::ostream& operator<<(std::ostream& ostr, const std::list<int>& list)
{
    for (auto &i : list) {
        ostr << " " << i;
    }
    return ostr;
}


template<class T>
class Sorter{
public:
    Sorter(): max_thread_count(std::thread::hardware_concurrency() - 1){}
//    Sorter(): max_thread_count(100){}
    ~Sorter(){
        end_of_data = true;
        for(auto &x: threads){
            if(x.joinable()){
                x.join();
            }
        }
    }

    std::list<T> do_sort(std::list<T> &v){
        if(v.empty()){
            return {};
        }
        std::list<T> res;
        res.splice(res.begin(), v, v.begin());  // 将原数据的第一个元素摘下，作为基准
        auto it = std::partition(v.begin(), v.end(), [&](const T &x){   // 将数据划成两份，左边比基准小，右边比基准大
            return x < res.front();
        });

        chunk_to_sort low;
        low.data.splice(low.data.end(), v, v.begin(), it);  // 将小的部分分给low
        auto future_low = low.prom.get_future();
        chunks.push(std::move(low));            // 提交任务：把数据压出栈中等待处理
        // 如果当前的线程数小于硬件支持的最大线程数，就启动一个线程(但处理的不一定是刚才划分的数据)
        if(threads.size() < max_thread_count){
            threads.emplace_back(&Sorter<T>::sort_thread, this);
        }
        auto r{do_sort((v))};   // 以同样的方法处理另一部分的数据

        // 整合结果
        res.splice(res.end(), r);   // 大的部分（已排序）
        while(future_low.wait_for(std::chrono::seconds(0)) != std::future_status::ready){
            try_sort_chunk();
        }
        res.splice(res.begin(), future_low.get());  // 小的部分（已排序）
        return res;
    }

private:
    struct chunk_to_sort{
        chunk_to_sort() = default;
        chunk_to_sort(const chunk_to_sort &c){
            data = c.data;
        }
        std::list<T> data;
        std::promise<std::list<T>> prom;
    };
    threadsafe_stack<chunk_to_sort> chunks;
    std::vector<std::thread> threads;
    const size_t max_thread_count;
    std::atomic<bool> end_of_data = false;

    void sort_chunk(const std::shared_ptr<chunk_to_sort> &chunk){
        chunk->prom.set_value(do_sort(chunk->data));
    }

    void try_sort_chunk(){
        std::shared_ptr<chunk_to_sort> chunk = chunks.pop();
        if(chunk){
            sort_chunk(chunk);
        }
    }

    void sort_thread(){
        while(!end_of_data){
            try_sort_chunk();
            std::this_thread::yield();  // 让出时间片中的剩余时间
        }
    }
};

template <typename T>
std::list<T> parallel_quick_sort(std::list<T> v) {
    if (v.empty()) {
        return {};
    }
    return Sorter<T>{}.do_sort(v);
}


int main(){
    std::uniform_int_distribution dist(0, 100000);
    std::default_random_engine engine;

    std::list<int> l;
    for(int i = 0; i < 100; i++){
        l.push_back(dist(engine));
    }

    auto res = parallel_quick_sort(l);
    std::cout << res << std::endl;

    return 0;
}