cmake_minimum_required(VERSION 3.20)

project(canmap C)

set(CMAKE_C_STANDARD 11)

add_executable(canmap
        src/main.c
        src/canmap.c
)