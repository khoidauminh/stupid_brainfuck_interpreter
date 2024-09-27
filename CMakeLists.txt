cmake_minimum_required(VERSION 3.13)

project(brainfuck_interpreter)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -g")

include_directories(${PROJECT_SOURCE_DIR})

add_executable(brainfuck brainfuck.c)

