//
// Created by chen on 2022/8/31.
//
// 基于单链表、无虚节点的单线程队列

#include <memory>
#include <iostream>
#include <cassert>

template<class T>
class queue{
private:
    struct node{
        T data;
        std::unique_ptr<node> next;
        node(T data_): data(std::move(data_)){}
    };
    std::unique_ptr<node> head;
    node *tail;

public:
    queue(): tail(nullptr){}
    queue(const queue &q) = delete;
    queue& operator=(const queue &q) = delete;

    std::shared_ptr<T> try_pop(){
        if(!head){
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> const res(std::make_shared<T>(std::move(head->data)));
        std::unique_ptr<node> const old_head = std::move(head);
        head = std::move(old_head->next);
        if(!head)
            tail = nullptr;
        return res;
    }

    void push(T new_value){
        std::unique_ptr<node> p(new node(std::move(new_value)));
        node * const new_tail = p.get();
        if(tail){
            tail->next = std::move(p);
        }else{
            head = std::move(p);
        }
        tail = new_tail;
    }
};

int main(){
    queue<int> q;
    q.push(10);
    q.push(20);
    q.push(30);
    auto p = q.try_pop();
    std::cout << *p << std::endl;
    p = q.try_pop();
    std::cout << *p << std::endl;
    p = q.try_pop();
    std::cout << *p << std::endl;
    p = q.try_pop();
    assert(p);          // fail
}