cmake_minimum_required(VERSION 3.28)

project(huffman_archiver)
enable_testing()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_subdirectory(src)
add_subdirectory(tests)
