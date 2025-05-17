cmake_minimum_required(VERSION 3.31)
project(lexer C)

set(CMAKE_C_STANDARD 11)

add_executable(lexer
        src/lexer.c
        src/tokenizer/tokenizer.h
        src/tokenizer/tokenizer.c
)

target_link_libraries(lexer
        ${CMAKE_SOURCE_DIR}/lib/jsonwriter/libjsonwriter.a
)

target_include_directories(lexer PUBLIC
        lib/jsonwriter/include
)
