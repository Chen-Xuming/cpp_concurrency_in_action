//
// Created by chen on 2022/9/5.
//

#ifndef CPP_CONCURRENCY_IN_ACTION_FUNCTION_WRAPPER_H
#define CPP_CONCURRENCY_IN_ACTION_FUNCTION_WRAPPER_H

#include <memory>

// 函数封装器
class function_wrapper{
public:
    template<class Function>
    function_wrapper(Function &&f): impl(new impl_type<Function>(std::move(f))){}
    function_wrapper() = default;
    function_wrapper(function_wrapper &&other): impl(std::move(other.impl)){}
    function_wrapper& operator=(function_wrapper &&other){
        impl = std::move(other.impl);
        return *this;
    }
    function_wrapper(const function_wrapper&) = delete;
    function_wrapper(function_wrapper&) = delete;
    function_wrapper& operator=(const function_wrapper&) = delete;

    void operator()(){
        impl->call();
    }

private:
    struct impl_base{
        virtual void call() = 0;
        virtual ~impl_base(){}
    };

    std::unique_ptr<impl_base> impl;

    template<class Function>
    struct impl_type: impl_base{
        Function f;
        explicit impl_type(Function &&f_): f(std::move(f_)){}
        void call(){
            f();
        }
    };
};

#endif //CPP_CONCURRENCY_IN_ACTION_FUNCTION_WRAPPER_H
