cmake_minimum_required(VERSION 3.8)
project(ss_server)

set(CMAKE_C_STANDARD 11)

set(SOURCE_FILES main.c logger.h logger.c)
add_executable(ss_server ${SOURCE_FILES})