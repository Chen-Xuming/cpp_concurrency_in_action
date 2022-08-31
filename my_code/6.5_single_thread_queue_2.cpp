//
// Created by chen on 2022/8/31.
//
// 带虚节点的队列
#include <memory>
#include <iostream>
#include <cassert>

template<typename T>
class queue
{
private:
    struct node
    {
        std::shared_ptr<T> data;        // data改用指针，分配内存的操作能够脱离锁的保护，缩短了互斥的持有时长
        std::unique_ptr<node> next;
    };

    std::unique_ptr<node> head;
    node* tail;

public:
    queue():head(new node), tail(head.get()){}

    queue(const queue& other)=delete;
    queue& operator=(const queue& other)=delete;

    std::shared_ptr<T> try_pop()
    {
        if(head.get()==tail)    // 新的判空条件：head = tail = 虚节点
        {
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> const res(head->data);
        std::unique_ptr<node> const old_head=std::move(head);
        head=std::move(old_head->next);
        return res;
    }

    void push(T new_value)
    {
        std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
        std::unique_ptr<node> p(new node);
        tail->data=new_data;
        node* const new_tail=p.get();
        tail->next=std::move(p);
        tail=new_tail;
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
