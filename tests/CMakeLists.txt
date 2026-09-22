# Создание исполняемого файла из main-файла
add_executable(run_huffman_tests tests.c)

# Связка исполняемого файла с библиотекой
target_link_libraries(run_huffman_tests PRIVATE huffman_lib)

# Указание заголовочного файла
target_include_directories(run_huffman_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR/src})

add_test(NAME huffman_tests COMMAND run_huffman_tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})