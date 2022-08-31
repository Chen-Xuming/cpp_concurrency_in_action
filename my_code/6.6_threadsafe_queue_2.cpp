//
// Created by chen on 2022/8/31.
//
// 带有精细粒度锁的线程安全队列

#include <iostream>
#include <thread>
#include <memory>
#include <mutex>

template<class T>
class threadsafe_queue{
private:
    struct node{
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };

    std::unique_ptr<node> head;
    node *tail;
    std::mutex head_mutex;
    std::mutex tail_mutex;

    node* get_tail(){
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        return tail;
    }

    std::unique_ptr<node> pop_head(){
        // 从头到位都要访问head，所以一开始就锁住head
        std::lock_guard<std::mutex> head_lock(head_mutex);

        // pop_head() 里面只有判空的时候需要访问tail，所以只需要在get_tail里面锁住tail就行
        if(head.get() == get_tail()){
            return nullptr;
        }

        std::unique_ptr<node> old_head = std::move(head);
        head = std::move(old_head->next);
        return old_head;
    }

public:
    threadsafe_queue(): head(new node), tail(head.get()){}
    threadsafe_queue(const threadsafe_queue &other) = delete;
    threadsafe_queue& operator=(const threadsafe_queue &other) = delete;

    std::shared_ptr<T> try_pop(){
        std::unique_ptr<node> old_head = pop_head();    // head加锁、解锁
        if(old_head){
            std::cout << "[pop] " << *(old_head->data) << std::endl;
        }
        return old_head ? old_head->data : std::shared_ptr<T>();
    }

    void push(T new_value){
        std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));

        // 创建新的虚节点当作新的队尾
        std::unique_ptr<node> p(new node);
        node * const new_tail = p.get();

        // 访问tail的部分才上锁
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        tail->data = new_data;
        tail->next = std::move(p);
        tail = new_tail;
        std::cout << "[push] " << *new_data << std::endl;
    }
};

threadsafe_queue<int> q;
void write(){
    int num = 100;
    for(int i = 0; i < 10; i++){
        q.push(num);
        num++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void read(){
    for(int i = 0; i < 10; i++){
        q.try_pop();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

int main(){

    std::thread t1(write), t2(read);
    t1.join();
    t2.join();
    return 0;
}