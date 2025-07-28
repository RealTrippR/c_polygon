cmake_minimum_required(VERSION 3.10)


# set the project name and version
project(C_POLYGON VERSION 1.0)

# specify the C standard
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED True)


include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/c_polygon ${CMAKE_CURRENT_SOURCE_DIR}/ASM)


add_executable(c_polygon
    c_polygon.c 
    Tests/readTest.c
    ASM/streql_x64_win.obj
    ASM/strneql_x64_win.obj
)

# Set warnings to treat as errors
if (MSVC) 
    target_compile_options(c_polygon PRIVATE
        /W4               
        /we4047           # 'different levels of indirection'
        /we4715           # 'not all control paths return a value'
        /we4716           # 'must return a value'
        /we4098           # 'void value not ignored as it ought to be'
        /we4777           # 'format string mismatch'
        /we4090           # 'different qualifiers'
    )
else()
    target_compile_options(c_polygon PRIVATE
        -Wall
        -Werror=return-type
        -Werror=incompatible-pointer-types
        -Werror=format
        -Werror=format-security
        -Werror=discarded-qualifiers
    )
endif()


# Set compiler flags
target_compile_options(c_polygon PRIVATE
    -std=c89
    -O3
    -DNDEBUG
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