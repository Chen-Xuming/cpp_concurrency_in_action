//
// Created by chen on 2022/8/23.
//

#include <queue>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <iostream>

template<class T>
class threadsafe_queue{
private:
    mutable std::mutex mtx;
    std::queue<T> data_queue;
    std::condition_variable data_cond;

public:
    threadsafe_queue() = default;
    ~threadsafe_queue() = default;
    threadsafe_queue(threadsafe_queue const &other){
        std::lock_guard<std::mutex> lk(other.mtx);
        data_queue = other.data_queue;
    }

    void push(T &val){
        std::lock_guard<std::mutex> lk(mtx);
        std::cout << "push: " << val << std::endl;
        data_queue.push(val);
        val++;
        data_cond.notify_one();
    }

    void wait_and_pop(T &val){
        std::unique_lock<std::mutex> lk(mtx);
        data_cond.wait(lk, [this]{return !data_queue.empty();});
        val = data_queue.front();
        std::cout << "pop: " << val << std::endl;
        data_queue.pop();
    }

    std::shared_ptr<T> wait_and_pop(){
        std::lock_guard<std::mutex> lk(mtx);
        data_cond.wait(lk, [this]{return !data_queue.empty();});
        std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
        data_queue.pop();
        return res;
    }

    bool try_pop(T &val){
        std::lock_guard<std::mutex> lk(mtx);
        if(data_queue.empty()){
            return false;
        }
        val = data_queue.front();
        data_queue.pop();
        return true;
    }

    std::shared_ptr<T> try_pop(){
        std::lock_guard<std::mutex> lk(mtx);
        if(data_queue.empty()){
            return nullptr;
        }
        std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
        data_queue.pop();
        return res;
    }

    bool empty() const{
        std::lock_guard<std::mutex> lk(mtx);
        return data_queue.empty();
    }
};

void read(threadsafe_queue<int> &q){
    int num;
    q.wait_and_pop(num);
}

void write(threadsafe_queue<int> &q, int &data){
    q.push(data);
}

int main(){

    int data = 1000;
    threadsafe_queue<int> q;
    std::thread w[5];
    for(int i = 0; i < 5; i++){
        w[i] = std::thread(write, std::ref(q), std::ref(data));
    }

    std::thread r[5];
    for(int i = 0; i < 5; i++){
        r[i] = std::thread(read, std::ref(q));
    }

    for(int i = 0; i < 5; i++){
        w[i].join();
        r[i].join();
    }

    return 0;
}