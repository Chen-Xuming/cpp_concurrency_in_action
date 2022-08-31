//
// Hierarchical Mutex
//

#include <thread>
#include <mutex>
#include <iostream>
#include <exception>

class hierarchical_mutex{
private:
    std::mutex internal_mutex;
    unsigned long const hierarchy_value;    // value of internal_mutex
    unsigned long previous_hierarchy_value;
    static thread_local unsigned long this_thread_hierarchy_value;

    void check_hierarchy_violation(){
        if(hierarchy_value >= this_thread_hierarchy_value){
            throw std::logic_error("Mutex hierarchy violated.");
        }
    }

    void update_hierarchy_value(){
        previous_hierarchy_value = this_thread_hierarchy_value;
        this_thread_hierarchy_value = hierarchy_value;
    }

public:
    hierarchical_mutex(unsigned long value): hierarchy_value(value), previous_hierarchy_value(0){}

    void lock(){
        check_hierarchy_violation();
        internal_mutex.lock();
        update_hierarchy_value();
    }

    void unlock(){
        if(this_thread_hierarchy_value != hierarchy_value){
            throw std::logic_error("Mutex hierarchy violated.");
        }
        internal_mutex.unlock();
        update_hierarchy_value();
    }

    bool try_lock(){
        check_hierarchy_violation();
        if(!internal_mutex.try_lock())
            return false;
        update_hierarchy_value();
        return true;
    }
};

thread_local unsigned long hierarchical_mutex::this_thread_hierarchy_value(ULONG_MAX);


hierarchical_mutex high_mtx(10000);
hierarchical_mutex low_mtx(5000);
hierarchical_mutex mid_mtx(6000);

void do_low_level_job(){
    std::cout << "do_low_level_job" << std::endl;
}
void low_level_func(){
    std::lock_guard<hierarchical_mutex> guard(low_mtx);
    do_low_level_job();
}

void do_high_level_job(){
    std::cout << "do_high_level_job" << std::endl;
}
void high_level_func(){
    std::lock_guard<hierarchical_mutex> guard(high_mtx);
    do_high_level_job();
    do_low_level_job();
}

void do_mid_level_jod(){
    std::cout << "do_mid_level_job" << std::endl;
}
void func(){
    std::lock_guard<hierarchical_mutex> guard(mid_mtx);
    high_level_func();
    do_mid_level_jod();
}

int main(){
    std::thread thread_a(high_level_func);
    thread_a.join();

    // logic_error !
    std::thread thread_b(func);
    thread_b.join();
    return 0;
}


