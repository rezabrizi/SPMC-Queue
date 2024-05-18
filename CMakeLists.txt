cmake_minimum_required(VERSION 3.27)
project(SMPC_Queue)

set(CMAKE_CXX_STANDARD 17)

add_executable(SMPC_Queue main.cpp
        smpc_q.h)
