//
// Created by chen on 2022/8/31.
//
// 带有精细粒度锁的线程安全队列-最终版

#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <memory>

template<class T>
class threadsafe_queue{
public:
    threadsafe_queue(): head(new node), tail(head.get()){}
    threadsafe_queue(const threadsafe_queue &q) = delete;
    threadsafe_queue& operator=(const threadsafe_queue &q) = delete;

    void push(T new_value){
        std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));     // 新数据
        std::unique_ptr<node> p(new node);  // 新的虚节点

        {
            std::lock_guard<std::mutex> tail_lock(tail_mutex);
            tail->data = new_data;
            node * const new_tail = p.get();
            tail->next = std::move(p);
            tail = new_tail;
//            std::cout << "[push] " << *new_data << std::endl;
        }   // 使用代码块{}加速tail_lock解锁

        data_cond.notify_one();
    }

    std::shared_ptr<T> wait_and_pop(){
        std::unique_ptr<node> const old_head = wait_pop_head();
//        std::cout << "[wait_pop] " << *old_head->data << std::endl;
        return old_head->data;
    }

    void wait_and_pop(T &value){
        std::unique_ptr<node> const old_head = wait_pop_head(value);
    }

    std::shared_ptr<T> try_pop(){
        std::unique_ptr<node> old_head = try_pop_head();
        if(old_head){
            std::cout << "[try_pop] " << *old_head->data << std::endl;
        }
        return old_head ? old_head->data : std::shared_ptr<T>();
    }

    bool try_pop(T &val){
        std::unique_ptr<node> const old_head = try_pop_head(val);
        return old_head != nullptr;
    }

    bool empty(){
        std::lock_guard<std::mutex> head_lock(head_mutex);
        return (head.get() == get_tail());
    }

private:
    struct node{
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };
    std::unique_ptr<node> head;
    node *tail;
    std::mutex head_mutex, tail_mutex;
    std::condition_variable data_cond;

    node* get_tail(){
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        return tail;
    }

    // 在其它函数中调用，调用前已经对 head 加锁，所以这里不用加锁
    std::unique_ptr<node> pop_head(){
        std::unique_ptr<node> old_head = std::move(head);
        head = std::move(old_head->next);
        return old_head;
    }

    // wait_and_pop() --> wait_pop_head() --> wait_for_data()
    // return head_lock --> lock() & pop_head()
    std::unique_lock<std::mutex> wait_for_data(){
        std::unique_lock<std::mutex> head_lock(head_mutex);
        data_cond.wait(head_lock, [&]{
            return head.get() != get_tail();
        });
        return std::move(head_lock);
    }

    std::unique_ptr<node> wait_pop_head(){
        std::unique_lock<std::mutex> head_lock(wait_for_data());
        return pop_head();
    }

    std::unique_ptr<node> wait_pop_head(T& value){
        std::unique_lock<std::mutex> head_lock(wait_for_data());
        value = std::move(*(head->data));
        return pop_head();
    }

    std::unique_ptr<node> try_pop_head(){
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if(head.get() == get_tail()){
            return std::unique_ptr<node>();
        }
        return pop_head();
    }

    std::unique_ptr<node> try_pop_head(T &val){
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if(head.get() == get_tail()){
            return std::unique_ptr<node>();
        }
        val = std::move(*head->data);
        return pop_head();
    }
};


//threadsafe_queue<int> q;
//
//void write(){
//    int num = 100;
//    for(int i = 0; i < 10; i++){
//        q.push(num);
//        num++;
//        std::this_thread::sleep_for(std::chrono::milliseconds(100));
//    }
//}
//
//void try_read(){
//    for(int i = 0; i < 5; i++){
//        q.try_pop();
//        std::this_thread::sleep_for(std::chrono::milliseconds(500));
//    }
//}
//
//void wait_read(){
//    for(int i = 0; i < 5; i++){
//        q.wait_and_pop();
//        std::this_thread::sleep_for(std::chrono::milliseconds(500));
//    }
//}
//
//int main(){
//    std::thread t1(write), t2(try_read), t3(wait_read);
//    t1.join();
//    t2.join();
//    t3.join();
//    return 0;
//}
