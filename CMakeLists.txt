cmake_minimum_required(VERSION 3.28)

project(huffman_archiver)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
