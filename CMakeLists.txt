cmake_minimum_required(VERSION 3.14)

project(knight_cpp2 VERSION 0.1 LANGUAGES CXX)

add_executable(knight)
set_target_properties(knight PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_sources(knight PRIVATE
    src/env.cpp
    src/env.hpp
    src/error.hpp
    src/funcs.cpp
    src/funcs.hpp
    src/lexer.cpp
    src/lexer.hpp
    src/main.cpp
    src/parser.cpp
    src/parser.hpp
    src/sourcepos.hpp
    src/value.hpp
    src/value.cpp
)

if (MSVC)
    target_compile_options(knight PRIVATE 
        /W4 /permissive-)
else()
    target_compile_options(knight PRIVATE 
        -Wall -Wextra -pedantic -Wsign-conversion)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(knight PRIVATE
            -fsanitize=undefined,address)
        target_link_libraries(knight PRIVATE
            -fsanitize=undefined,address)
    endif()
endif()
