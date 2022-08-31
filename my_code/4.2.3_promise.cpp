//
// Created by chen on 2022/8/24.
//

#include <iostream>
#include <future>
#include <thread>
#include <string>

void f(std::future<std::string> *future){
    std::cout << future->get() << std::endl;    // promise.set_value()之前，get()阻塞
}

int main(){
    std::promise<std::string> promise;
    std::future<std::string> future = promise.get_future();
    std::thread t(f, &future);
    promise.set_value("hello");
    t.join();
    return 0;
}