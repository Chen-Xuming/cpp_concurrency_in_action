//
// Created by chen on 2022/8/24.
//

#include <future>
#include <iostream>

int exp(int num, int e){
    int res = 1;
    for(int i = 0; i < e; i++){
        res *= num;
    }
    std::cout << "calculate exp done." << std::endl;
    return res;
}

void do_something(){
    std::cout << "do_something." << std::endl;
}

int main(){
    std::future<int> ans = std::async(std::launch::deferred, exp, 2, 10);
    do_something();
    std::cout << "ans = " << ans.get() << std::endl;
    return 0;
}