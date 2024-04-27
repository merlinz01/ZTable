cmake_minimum_required(VERSION 3.25)

project(ZTable C)

set(CMAKE_C_STANDARD 11)

add_library(ZTable SHARED ztable.c)

target_link_libraries(ZTable comctl32)

set_target_properties(ZTable PROPERTIES PREFIX "")

add_custom_command(TARGET ZTable POST_BUILD COMMAND copy ZTable.dll .\\..\\ZTable.dll)
add_custom_command(TARGET ZTable POST_BUILD COMMAND copy ZTable.dll .\\..\\..\\Flatland\\ZTable.dll)
add_custom_command(TARGET ZTable POST_BUILD COMMAND copy ZTable.dll .\\..\\..\\schedule\\ZTable.dll)