# Создание статической библиотеки
add_library(huffman_lib STATIC huffman.c)

# Указание заголовочного файла
target_include_directories(huffman_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Создание исполняемого файла из main-файла
add_executable(huffman main.c)

# Связка исполняемого файла с библиотекой
target_link_libraries(huffman PRIVATE huffman_lib)
