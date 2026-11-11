cmake_minimum_required(VERSION 3.10)
project(lab2 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)

add_executable(child Child.c SharedData.h)
add_executable(main Main.c SharedData.h)