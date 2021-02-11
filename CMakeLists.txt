cmake_minimum_required(VERSION 3.15)
project(ZOS_SEM_2020)


set(CMAKE_CXX_STANDARD 17)

add_executable(ZOS_SEM_2020 main.cpp Commands.h Commands.cpp FileSystem.h FileSystem.cpp)