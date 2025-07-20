cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(C_POLYGON VERSION 1.0)

# specify the C standard
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED True)


include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/c_polygon ${CMAKE_CURRENT_SOURCE_DIR}/ASM)


add_executable(c_polygon
    c_polygon.c 
    Tests/test.c
    ASM/streql_x64_win.obj
    ASM/strneql_x64_win.obj
)

# Set compiler flags
target_compile_options(c_polygon PRIVATE
    -std=c89
    -O3
    -msse4.2
    -march=native
    -funroll-loops
    -fpeel-loops
    -fpredictive-commoning
    -frename-registers
    -ffast-math
    -flto
    -Wall
)

target_link_options(c_polygon PRIVATE
    -flto
)