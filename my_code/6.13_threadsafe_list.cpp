//
// Created by chen on 2022/9/1.
//
// 线程安全链表

#include <memory>
#include <mutex>
#include <utility>
#include <iostream>
#include <thread>

template <class T>
class ConcurrentList{
public:
    ConcurrentList() = default;
    ~ConcurrentList() = default;
    ConcurrentList(const ConcurrentList&) = delete;
    ConcurrentList& operator=(const ConcurrentList&) = delete;

    void push_front(const T& x){
        std::unique_ptr<Node> t(new Node(x));
        std::lock_guard<std::mutex> head_lock(head_.m);
        t->next = std::move(head_.next);
        head_.next = std::move(t);
        std::cout << "[push_front] " << x << std::endl;
    }

    template<class Function>
    void for_each(Function f){
        Node *cur = &head_;
        std::unique_lock<std::mutex> head_lock(head_.m);
        while(Node* const next = cur->next.get()){
            std::unique_lock<std::mutex> next_lock(next->m);
            head_lock.unlock();
            f(*next->data);
            cur = next;
            head_lock = std::move(next_lock);
        }
    }

    template<class Function>
    std::shared_ptr<T> find_first_if(Function F){
        Node *cur = &head_;
        std::unique_lock<std::mutex> head_lock(head_.m);
        while(Node* const next = cur->next.get()){
            std::unique_lock<std::mutex> next_lock(next->m);
            head_lock.unlock();
            if(f(*next->data)){
                return next->data;
            }
            cur = next;
            head_lock = std::move(next_lock);
        }
        return nullptr;
    }

    template<class Function>
    void remove_if(Function f){
        Node *cur = &head_;
        std::unique_lock<std::mutex> head_lock(head_.m);
        while(Node* const next = cur->next.get()) {
            std::unique_lock<std::mutex> next_lock(next->m);
            if (f(*next->data)) {
                auto old_next = std::move(cur->next);
                cur->next = std::move(next->next);
                next_lock.unlock();
            }else{
                head_lock.unlock();
                cur = next;
                head_lock = std::move(next_lock);
            }
        }
    }

private:
    struct Node{
        std::mutex m;   // 一个节点一把锁
        std::shared_ptr<T> data;
        std::unique_ptr<Node> next;
        Node() = default;
        Node(const T &x): data(std::make_shared<T>(x)){}
    };
    Node head_;
};

ConcurrentList<int> concurrentList;

void write(){
    int num = 100;
    for(int i = 0; i < 10; i++){
        concurrentList.push_front(num);
        num++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void print(int x){
    std::cout << x << " ";
}

bool judge(int x){
    return x % 2 == 0;
}

void read(){
    for(int i = 0; i < 10; i++){
        std::cout << "[print] ";
        concurrentList.for_each(print);
        std::cout << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

void remove_data(){
    for(int i = 0; i < 5; i++){
        std::cout << "[delete]" << std::endl;
        concurrentList.remove_if(judge);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

int main(){
    std::thread t1(write), t2(read);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::thread t3(remove_data);
    t1.join();
    t2.join();
    t3.join();
    return 0;
}














