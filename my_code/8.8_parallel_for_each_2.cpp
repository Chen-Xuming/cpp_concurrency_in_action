//
// Created by chen on 2022/9/2.
//
// for_each并发版本 使用 async 实现

#include <algorithm>
#include <iostream>
#include <future>
#include <vector>

template <class Iterator, class Function>
void parallel_for_each(Iterator first, Iterator last, Function f){
    unsigned long const length = std::distance(first, last);
    if(length == 0){
        return;
    }

    unsigned long const min_per_thread = 25;
    if(length < 2 * min_per_thread){
        std::for_each(first, last, f);
    }
    else{
        auto const mid = first + length / 2;
        std::future<void> first_half = std::async(parallel_for_each<Iterator, Function>, first, mid, f);
        parallel_for_each(mid, last, f);
        first_half.get();       // 将子线程的异常传给主线程
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