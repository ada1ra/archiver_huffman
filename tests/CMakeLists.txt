# Указание заголовочного файла
target_include_directories(run_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Создание исполняемого файла из main-файла
add_executable(run_tests tests.c)

# Связка исполняемого файла с библиотекой
target_link_libraries(run_tests PRIVATE huffman_lib)
