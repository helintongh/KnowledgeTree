cmake_minimum_required(VERSION 3.16)
# ${CMAKE_CURRENT_SOURCE_DIR}获取cmake源文件路径
get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})
project(${ProjectId} C)

set(CMAKE_C_STANDARD 11)
# 获取每一个可执行程序,每一个可执行程序都通过add_executable添加到编译流程里
file(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
foreach(file ${files})
    get_filename_component(name ${file} NAME)
    add_executable(${name} ${file})
endforeach()