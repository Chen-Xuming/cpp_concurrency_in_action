//
// Created by chen on 2022/9/2.
//
// for_each函数并行版本1：
// 使用 hardware_concurrency 决定线程数，使用连续数据块避免伪共享，使用 std::packaged_task 和 std::future 在线程间传递异常

#include <iostream>
#include <thread>
#include <future>
#include <vector>
#include <algorithm>

// 利用RAII机制保证所有线程都能join
class join_threads{
private:
    std::vector<std::thread> &threads;
public:
    explicit join_threads(std::vector<std::thread> &threads_): threads(threads_){}
    ~join_threads(){
        for(auto &t : threads){
            if(t.joinable()){
                t.join();
            }
        }
    }
};

template<class Iterator, class Function>
void parallel_for_each(Iterator first, Iterator last, Function f){
    // 数据划分，确定线程个数
    unsigned long const length = std::distance(first, last);
    if(length == 0 ) return;
    unsigned long const min_per_thread = 25;
    unsigned long const max_threads = length / min_per_thread + (length % min_per_thread == 0 ? 0 : 1);
    unsigned long const hardware_threads = std::thread::hardware_concurrency();
    unsigned long const num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
    unsigned long const block_size = length / num_threads;
    std::cout << "num of threads: " << num_threads << std::endl;

    std::vector<std::thread> threads(num_threads - 1);
    std::vector<std::future<void>> futures(num_threads - 1);
    join_threads joiner(threads);

    Iterator block_start = first;
    for(int i = 0; i < num_threads - 1; i++){
        Iterator block_end = block_start;
        std::advance(block_end, block_size);
        std::packaged_task<void(void)> task([=](){
            std::for_each(block_start, block_end, f);
        });
        futures[i] = task.get_future();
        threads[i] = std::thread(std::move(task));
        block_start = block_end;
    }
    std::for_each(block_start, last, f);

    // 获取子线程抛出的异常
    for(auto &fut : futures){
        fut.get();
    }
}

void print(int num){
    std::cout << num << " ";
}

int main(){
    std::vector<int> vec(510);
    for(int i = 0; i < vec.size(); i++){
        vec[i] = i + 1;
    }

    parallel_for_each(vec.begin(), vec.end(), print);
    std::cout << std::endl;
}



















