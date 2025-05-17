cmake_minimum_required(VERSION 3.31)
project(lexer C)

set(CMAKE_C_STANDARD 11)

add_library(lexer STATIC
        src/tokenizer/tokenizer.h
        src/tokenizer/tokenizer.c
)

target_include_directories(lexer PUBLIC
        lib/jsonwriter/include
)

add_executable(lexer_cli
        src/lexer.c
        src/tokenizer/tokenizer.h
        src/tokenizer/tokenizer.c
)

target_link_libraries(lexer_cli
        ${CMAKE_SOURCE_DIR}/lib/jsonwriter/libjsonwriter.a
)

target_include_directories(lexer_cli PUBLIC
        lib/jsonwriter/include
)
