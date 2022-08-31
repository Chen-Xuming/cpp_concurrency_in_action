/*
 *      并发求和
 */

#include <iostream>
#include <thread>
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>

template<class Iterator, class T>
struct accumulate_block{
    void operator()(Iterator begin, Iterator end, T &result){
        result = std::accumulate(begin, end, result);
    }
};

template<class Iterator, class T>
T parallel_accumulate(Iterator begin, Iterator end, T init){
    unsigned int const length = std::distance(begin, end);

    // 确定线程的数量
    unsigned int const min_per_thread = 25;
    unsigned int const max_threads = length / min_per_thread + (length % min_per_thread == 0 ? 0 : 1);
    unsigned int const hardware_threads = std::thread::hardware_concurrency();
    int thread_num = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
    std::cout << "thread_hardware = " << hardware_threads << std::endl;
    std::cout << "thread_num = " << thread_num << std::endl;

    // 数据划分与启动线程
    unsigned int block_size = length / thread_num;
    std::vector<T> results(thread_num);
    std::vector<std::thread> threads(thread_num - 1);   // 减1是因为主线程也是一个参与计算的线程
    Iterator block_start = begin;
    for(int i = 0; i < thread_num - 1; i++){
        Iterator block_last = block_start;
        std::advance(block_last, block_size);
        threads[i] = std::thread(accumulate_block<Iterator, T>(), block_start, block_last, std::ref(results[i]));
        block_start = block_last;
    }
    accumulate_block<Iterator, T>()(block_start, end, std::ref(results[thread_num - 1]));

    // 各线程保证在结果汇总之前完成计算即可
    for (auto &t : threads){
        t.join();
    }

    return std::accumulate(results.begin(), results.end(), init);
}

int main(){
    std::random_device rd;
    std::default_random_engine rand_eng(rd());
    std::uniform_int_distribution<int> dist_int(0, 100);
    std::uniform_real_distribution<float> dist_float(-50.0, 100.0);

    std::vector<int> vec_int(1234);
    std::vector<float> vec_float(1234);

    for(int i = 0; i < vec_int.size(); i++){
        vec_int[i] = dist_int(rand_eng);
        vec_float[i] = dist_float(rand_eng);
    }

    int result_parallel_int = parallel_accumulate(vec_int.begin(), vec_int.end(), 0);
    float result_parallel_float = parallel_accumulate(vec_float.begin(), vec_float.end(), 0.0f);

    int result_int = std::accumulate(vec_int.begin(), vec_int.end(), 0);
    float result_float = std::accumulate(vec_float.begin(), vec_float.end(), 0.0f);

    std::cout << result_parallel_int << "   " << result_int << std::endl;
    std::cout << result_parallel_float << "   " << result_float << std::endl;

    return 0;
}