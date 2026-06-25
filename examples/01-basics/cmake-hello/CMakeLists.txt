cmake_minimum_required(VERSION 3.16)
project(cmake_hello C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(hello src/main.c)
target_link_libraries(hello m)

