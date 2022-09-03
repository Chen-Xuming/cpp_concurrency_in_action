//
// Created by chen on 2022/9/3.
//
// std::partial_sum() 并发版本

/*
示例

1 1 1 1 1 1 1 1 1 // 输入 9 个 1
// 划分为三部分
1 1 1
1 1 1
1 1 1
// 得到三个部分的结果
1 2 3
1 2 3
1 2 3
// 将第一部分的尾元素（即 3）加到第二部分
1 2 3
4 5 6
1 2 3
// 再将第二部分的尾元素（即 6）加到第三部分
1 2 3
4 5 6
7 8 9

*/


#include <algorithm>
#include <thread>
#include <future>
#include <numeric>
#include <iostream>

template<class Iterator>
void parallel_partial_sum(Iterator first, Iterator last){
    using value_type = typename Iterator::value_type;
    struct process_chunk{
        void operator()(Iterator begin, Iterator last,
                        std::future<value_type> *previous_end_value, std::promise<value_type> *end_value){
            try{
                Iterator end = last;
                ++end;
                std::partial_sum(begin, end, begin);    // 第三个参数是存储结果的起始位置

                // 如果不是第一块，则当前块的元素自增前一块的最后一个元素
                if(previous_end_value){
                    value_type addend = previous_end_value->get();

                    // 首先处理的是本块的最后一个元素，因为下一块（若存在）正在等待此元素
                    *last += addend;
                    if(end_value){
                        end_value->set_value(*last);
                    }
                    // 其次才处理其他元素
                    std::for_each(begin, last, [addend](value_type &item){
                        item += addend;
                    });
                }else if(end_value){    // 如果是第一块，只需要为下一块提供尾元素
                    end_value->set_value(*last);
                }
            }catch (...){       // 异常传给下一块，最后一块抛出异常
                if(end_value){
                    end_value->set_exception(std::current_exception());
                } else{
                    throw;
                }
            }
        }
    };

    std::size_t length = std::distance(first, last);
    if(length <= 1){
        return;
    }
    size_t min_per_thread = 25;
    size_t max_threads = (length + min_per_thread - 1) / min_per_thread;
    size_t hardware_threads = std::thread::hardware_concurrency();
    size_t num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
    size_t block_size = length / num_threads;

    std::vector<std::promise<value_type>> end_values(num_threads - 1);  // 存储块内的尾元素值
    std::vector<std::future<value_type>> prev_end_values;
    prev_end_values.reserve(num_threads - 1);    // 预留的空间

    Iterator block_start = first;
    std::vector<std::jthread> threads(num_threads - 1); //C++20
    for(size_t i = 0; i < num_threads - 1; ++i){
        Iterator block_last = block_start;
        std::advance(block_last, block_size - 1);   // 由于process_chunk的设计，block_last指向最后一个元素，而不是最后一个元素的后一个位置
        threads[i] = std::jthread(process_chunk{}, block_start, block_last,
                                  i != 0 ? &prev_end_values[i - 1] : nullptr,
                                  &end_values[i]);
        block_start = block_last;
        ++block_start;
        prev_end_values.emplace_back(end_values[i].get_future());
    }
    Iterator final_element = block_start;
    std::advance(final_element, std::distance(block_start, last) - 1);
    process_chunk{}(block_start, final_element, num_threads > 1 ? &prev_end_values.back() : nullptr, nullptr);
}

int main(){
    std::vector<int> vec(1000);
    for(int i = 0; i < vec.size(); i++){
        vec[i] = i+1;
    }
    parallel_partial_sum(vec.begin(), vec.end());
    for(int i = 0; i < vec.size(); i++){
        std::cout << vec[i] << "  ";
    }
    std::cout << std::endl;
}

















