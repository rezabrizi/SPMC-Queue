cmake_minimum_required(VERSION 3.27)
project(AtomicRing)

# Set Boost directory
set(BOOST_ROOT /opt/homebrew/opt/boost)

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use Clang Instead of GCC
set(CMAKE_C_COMPILER /opt/homebrew/opt/llvm/bin/clang)
set(CMAKE_CXX_COMPILER /opt/homebrew/opt/llvm/bin/clang++)

# ✅ Explicitly include the `src/` directory
include_directories(${CMAKE_SOURCE_DIR}/src)

# Define the executable
add_executable(AtomicRing src/main.cpp
        src/spmc_q.h
        src/benchmark.h
        src/benchmark.cpp
        src/spsc.h)

# Link against Boost if needed
target_link_libraries(AtomicRing ${Boost_LIBRARIES})
