cmake_minimum_required(VERSION 3.20)
project(more CXX)

# ⚠️ 关键：必须同时列出 main.cpp 和 io.cpp
add_executable(more_io_demo
        main.cpp
        io.cpp
)