cmake_minimum_required(VERSION 3.20)
project(cpp_stack_attack)

set(CMAKE_CXX_STANDARD 17)

link_libraries(ncurses)

add_executable(untitled7 main.cpp Frame.h Frame.cpp)

add_executable(curs test_1.c)

