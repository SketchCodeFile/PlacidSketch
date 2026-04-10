# CMakeLists.txt for XSketch Project
# 用于编译XSketch核心功能的测试程序

cmake_minimum_required(VERSION 3.14)
project(XSketch)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O2 -g")

# 添加项目根目录和Common目录到包含路径
include_directories(.)
include_directories(Common)

# Windows平台设置
if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++17")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pedantic -Wall -Wextra")
endif()

# 添加可执行文件
add_executable(XSketch main.cc)
