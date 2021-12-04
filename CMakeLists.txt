cmake_minimum_required(VERSION 3.20)
project(cpp_stack_attack)

set(CMAKE_CXX_STANDARD 17)

add_executable(curs heap_defence.c)

target_include_directories(curs PRIVATE flipperzero-firmware)

