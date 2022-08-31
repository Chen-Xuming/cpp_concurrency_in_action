#include <thread>
#include <iostream>

void do_something(int& i)
{
    ++i;
    if(i % 100000 == 0){
        std::cout << i << std::endl;
    }
}

struct func
{
    int& i;

    func(int& i_):i(i_){
        std::cout << "construct func.\n";
    }

    void operator()()
    {
        for(unsigned j=0;j<1000000;++j)
        {
            if(j % 100000 == 0)
                std::cout << "call do_somrthing." << std::endl;
            do_something(i);
        }
    }
};


void oops()
{
    int some_local_state=0;
    func my_func(some_local_state);
    std::thread my_thread(my_func);
    my_thread.detach();

    while(1){

    }
}

int main()
{
    oops();
}
