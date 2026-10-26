cmake_minimum_required(VERSION 3.10)
project(Algorithm)

set(CMAKE_CXX_STANDARD 17)

# 【关键修改】：在这里列出项目中所有的 .cpp 源文件
add_executable(algo_test
        main.cpp
        io.cpp
)