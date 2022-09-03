//
// Created by chen on 2022/9/2.
//
// 并发本版的 find()

#include <vector>
#include <iostream>
#include <future>
#include <thread>
#include <atomic>

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

template<class Iterator, class MatchType>
Iterator parallel_find(Iterator first, Iterator last, MatchType match){
    // 在局部数据上的查找过程
    struct find_element{
        void operator()(Iterator begin, Iterator end, MatchType match, std::promise<Iterator> *result, std::atomic<bool>* done_flag){
            try{
                // 每检查完一个元素，就查看done_flag一次，如果其它线程已经找到要找的元素，就停止查找
                for(; (begin != end) && !done_flag->load(); ++begin){
                    if(*begin == match){
                        result->set_value(begin);
                        done_flag->store(true);
                        return;
                    }
                }
            }catch (...){   // 捕获任意类型的异常
                try{
                    result->set_exception(std::current_exception());
                    done_flag->store(true);     // 发生异常也停止查找
                }catch (...){}  // 如果再出现异常，直接丢弃
            }
        }
    };

    // 划分数据，确定线程数量
    unsigned long const length = std::distance(first,last);
    if(length == 0) return last;
    unsigned long const min_per_thread = 25;
    unsigned long const max_threads = length / min_per_thread + (length % min_per_thread == 0 ? 0 : 1);
    unsigned long const hardware_threads = std::thread::hardware_concurrency();
    unsigned long const num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
    unsigned long const block_size = length / num_threads;

    std::promise<Iterator> result;
    std::atomic<bool> done_flag = false;
    std::vector<std::thread> threads(num_threads - 1);

    {
        join_threads joiner(threads);
        Iterator block_start = first;
        for(int i = 0; i < threads.size(); i++){
            Iterator block_end = block_start;
            std::advance(block_end, block_size);
            threads[i] = std::thread(find_element(), block_start, block_end, match, &result, &done_flag);
            block_start = block_end;
        }
        find_element()(block_start, last, match, &result, &done_flag);  // 在剩余数据中查找
    }   // {} 使joiner析构，确保threads全部join，才能保证下面的判断有意义

    if(!done_flag.load()){
        return last;
    }
    return result.get_future().get();   // 查找结果或异常
}

int main(){
    std::vector<int> vec(1010);
    for(int i = 0; i < vec.size(); i++){
        vec[i] = i+1;
    }

    auto res = parallel_find(vec.begin(), vec.end(), 312321);
    if(res == vec.end()){
        std::cout << "not found." << std::endl;
    } else{
        std::cout << "found: " << *res <<  std::endl;
    }

    return 0;
}
























