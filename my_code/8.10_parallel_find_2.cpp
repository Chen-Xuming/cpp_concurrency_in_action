//
// Created by chen on 2022/9/2.
//
// find()并发版本 基于std::async实现

#include <future>
#include <iostream>
#include <vector>
#include <atomic>

template<class Iterator, class MatchType>
Iterator parallel_find_impl(Iterator first, Iterator last, MatchType match, std::atomic<bool> &done_flag){
    try {
        unsigned long const length = std::distance(first, last);
        unsigned long const min_per_thread = 20;

        // 数量小于一定范围，直接搜索
        if(length < 2 * min_per_thread){
            for(; first != last && !done_flag.load(); ++first){
                if(*first == match){
                    done_flag.store(true);
                    return first;
                }
            }
            return last;
        }

        else{
            auto const mid = first + length / 2;
            std::future<Iterator> async_part_result = std::async(&parallel_find_impl<Iterator, MatchType>, mid, last, match,
                                                                 std::ref(done_flag));
            auto const direct_part_result = parallel_find_impl(first, mid, match, done_flag);

            // 若match不在前半部分，那么以下条件成立
            if(direct_part_result == mid){
                return async_part_result.get();
            }
            return direct_part_result;
        }
    }catch (...){
        done_flag = true;
        throw;
    }
}

template<class Iterator, class MatchType>
Iterator parallel_find(Iterator first, Iterator last, MatchType match){
    std::atomic<bool> done_flag = false;
    return parallel_find_impl(first, last, match, done_flag);
}

int main(){
    std::vector<int> vec(1010);
    for(int i = 0; i < vec.size(); i++){
        vec[i] = i+1;
    }

    auto res = parallel_find(vec.begin(), vec.end(), 998);
    if(res == vec.end()){
        std::cout << "not found." << std::endl;
    } else{
        std::cout << "found: " << *res <<  std::endl;
    }

    return 0;
}



