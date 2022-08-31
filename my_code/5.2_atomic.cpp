//
// Created by chen on 2022/8/27.
//

#include <atomic>
#include <cassert>

//struct A{
//    int a[100];
//};
//
//struct B{
//    int x, y;
//};

class A{};

int main(){
//    assert(std::atomic<A>{}.is_lock_free());
//    assert(std::atomic<B>{}.is_lock_free());

    A a[5];
    std::atomic<A*> p(a);     // p为 &a[0]
    A *x = p.fetch_add(2);    // p为 &a[2]，但返回值为 &a[0]
    assert(x == a);
    assert(p.load() == &a[2]);
//    x = (p -= 1);    // p 为 &a[1], 并返回给x, 等价于 x = p.fetch_sub(1) - 1;
    x = p.fetch_sub(1) - 1;
    assert(x == &a[1]);
    assert(p.load() == &a[1]);

    return 0;
}
