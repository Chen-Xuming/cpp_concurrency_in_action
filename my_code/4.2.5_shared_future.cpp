//
// Created by chen on 2022/8/24.
//

#include <iostream>
#include <thread>
#include <future>

void f(std::shared_future<int> &share, int id){
    auto sf = share;
    std::cout << "#" << id << " " << sf.get() << std::endl;
}

int main(){
    std::promise<int> promise;
    std::shared_future<int> sf = promise.get_future().share();
    std::thread threads[5];
    for(int i = 0; i < 5; i++){
        threads[i] = std::thread(f, std::ref(sf), i);
    }
    promise.set_value(100);
    for(auto & thread : threads){
        thread.join();
    }
    return 0;
}