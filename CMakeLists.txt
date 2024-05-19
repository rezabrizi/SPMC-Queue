cmake_minimum_required(VERSION 3.27)
project(SMPC_Queue)


# Set the Boost root directory
set(BOOST_ROOT /opt/homebrew/opt/boost)

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

set(CMAKE_CXX_STANDARD 17)

add_executable(benchmark src/main.cpp
        src/spmc_q.h
        src/benchmark.h
        src/benchmark.cpp
        )


target_link_libraries(benchmark ${Boost_LIBRARIES})