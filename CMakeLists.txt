cmake_minimum_required(VERSION 3.25)
project(ZTable C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(ZTable
        ztable.c
        ztable.h)
