cmake_minimum_required(VERSION 3.31)
project(Zenith)

set(CMAKE_CXX_STANDARD 20)

add_library(Zenith SHARED core/test.cpp)

target_include_directories(Zenith PUBLIC include)
target_compile_options(Zenith PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

add_executable(TriangleTest examples/triangle/main.cpp)
target_link_libraries(TriangleTest PRIVATE Zenith)
target_include_directories(TriangleTest PUBLIC include)

