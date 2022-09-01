cmake_minimum_required(VERSION 3.22)
project(cpp_concurrency_in_action)

set(CMAKE_CXX_STANDARD 17)

link_libraries(atomic)  # for Chap 5

add_executable(cpp_concurrency_in_action my_code/6.13_threadsafe_list.cpp)