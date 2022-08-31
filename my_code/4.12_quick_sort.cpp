//
// Created by chen on 2022/8/25.
//

#include <list>
#include <random>
#include <iostream>
#include <algorithm>
#include <future>
#include <chrono>

std::ostream& operator<<(std::ostream& ostr, const std::list<int>& list)
{
    for (auto &i : list) {
        ostr << " " << i;
    }
    return ostr;
}

template<class T>
std::list<T> my_quick_sort(std::list<T> input){
    if(input.empty()){
        return input;
    }

    std::list<T> result;
    result.splice(result.begin(), input, input.begin());    // 将input的首元素取下来，作为基准

    // 将input重新整理，前半部分小于pivot，后半部分大于等于pivot
    T const &pivot = *result.begin();
    auto divide_point = std::partition(input.begin(), input.end(), [&](T const &t){return t < pivot;}); // 返回指向第二部分的首部

    // 将两部分截取下来，分别进行快速排序
    std::list<int> lower_part;
    lower_part.splice(lower_part.begin(), input, input.begin(), divide_point);
    auto new_lower(my_quick_sort(std::move(lower_part)));
    auto new_higher(my_quick_sort(std::move(input)));

    result.splice(result.begin(), new_lower);
    result.splice(result.end(), new_higher);
    return result;
}

// 并行快速排序
template<class T>
std::list<T> my_parallel_quick_sort(std::list<T> input){
    if(input.empty()){
        return input;
    }

    std::list<T> result;
    result.splice(result.begin(), input, input.begin());    // 将input的首元素取下来，作为基准

    // 将input重新整理，前半部分小于pivot，后半部分大于等于pivot
    T const &pivot = *result.begin();
    auto divide_point = std::partition(input.begin(), input.end(), [&](T const &t){return t < pivot;}); // 返回指向第二部分的首部

    // 将两部分截取下来，分别进行快速排序
    std::list<int> lower_part;
    lower_part.splice(lower_part.begin(), input, input.begin(), divide_point);
    std::future<std::list<T>> new_lower(std::async(my_parallel_quick_sort<T>, std::move(lower_part)));     // 对左半部分采取并行排序
    auto new_higher(my_quick_sort(std::move(input)));

    result.splice(result.begin(), new_lower.get());
    result.splice(result.end(), new_higher);
    return result;
}

int main(){
    using namespace std::chrono;

    std::uniform_int_distribution dist(0, 100000);
    std::default_random_engine engine;

    std::list<int> l;
    for(int i = 0; i < 10000000; i++){
        l.push_back(dist(engine));
    }
    std::list<int> l2(l);

    std::cout << "------------- my_quick_sort -----------------" << std::endl;
    auto start1 = system_clock::now();
    l = my_quick_sort(l);
    auto end1 = system_clock::now();
    auto duration1 = duration_cast<seconds>(end1 - start1);
    std::cout << "time cost: " << duration1.count() << "s." << std::endl;

    std::cout << "------------- my_parallel_quick_sort -----------------" << std::endl;
    auto start2 = system_clock::now();
    l2 = my_quick_sort(l2);
    auto end2 = system_clock::now();
    auto duration2 = duration_cast<seconds>(end2 - start2);
    std::cout << "time cost: " << duration2.count() << "s." << std::endl;

    std::cout << "--------------------" << std::endl;
//    std::cout << "l = " << l << std::endl;
//    std::cout << "l2 = " << l2 << std::endl;
    if(l == l2){
        std::cout << "check." << std::endl;
    }

    return 0;
}