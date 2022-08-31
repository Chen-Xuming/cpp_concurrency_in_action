//
// thread-safe stack
//

#include <exception>
#include <memory>
#include <stack>
#include <mutex>
#include <iostream>

struct empty_stack: std::exception{
    const char* what() const noexcept override {
        return "Empty Stack.";
    }
};

template<class T>
class threadsafe_stack{
private:
    std::stack<T> data;
    mutable std::mutex mtx;
public:
    threadsafe_stack() = default;
    threadsafe_stack(const threadsafe_stack &other){
        std::lock_guard<std::mutex> guard(other.mtx);
        data = other.data;
    }
    threadsafe_stack& operator=(const threadsafe_stack&) = delete;

    void push(T value){
        std::lock_guard<std::mutex> guard(mtx);
        data.push(value);
    }

    std::shared_ptr<T> pop(){
        std::lock_guard<std::mutex> guard(mtx);
        if(data.empty()){
            throw empty_stack();
        }
        std::shared_ptr<T> const top(std::make_shared<T>(data.top()));
        data.pop();
        return top;
    }

    void pop(T &result){
        std::lock_guard<std::mutex> guard(mtx);
        if(data.empty()){
            throw empty_stack();
        }
        result = data.top();
        data.pop();
    }

    bool empty() const{
        std::lock_guard<std::mutex> guard(mtx);
        return data.empty();
    }
};

int main(){
    threadsafe_stack<int> stk;
    stk.push(100);
    stk.push(200);
    std::shared_ptr<int> p = stk.pop();
    std::cout << *p << std::endl;
    int result = 0;
    stk.pop(result);
    std::cout << result << std::endl;
    stk.pop();  // exception

    return 0;
}
