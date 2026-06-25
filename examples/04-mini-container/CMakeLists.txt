cmake_minimum_required(VERSION 3.16)
project(mini_container C)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "mini-container uses Linux namespaces and only builds on Linux")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(mini-container src/main.c)
target_compile_options(mini-container PRIVATE -Wall -Wextra -Wpedantic)
