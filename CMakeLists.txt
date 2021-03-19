cmake_minimum_required(VERSION 3.14)

project(knight_cpp2 VERSION 0.1 LANGUAGES CXX)
include(CheckIPOSupported)

add_executable(knight)
set_target_properties(knight PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_sources(knight PRIVATE
    src/debug.hpp
    src/emit.cpp
    src/emit.hpp
    src/env.cpp
    src/env.hpp
    src/error.hpp
    src/eval.cpp
    src/eval.hpp
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

if (WIN32)
    target_sources(knight PRIVATE src/shell_win32.cpp)
else()
    target_sources(knight PRIVATE src/shell_posix.cpp)
endif()

check_ipo_supported(RESULT IS_IPO_SUPPORTED)
set_target_properties(knight PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION $<AND:$<CONFIG:Release>,${IS_IPO_SUPPORTED}>)

set(SANITISERS "$<$<CONFIG:Debug>:-fsanitize=undefined,address;-fno-omit-frame-pointer>")

target_compile_options(knight PRIVATE
    "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-pedantic;-Wsign-conversion;${SANITISERS}>"
    "$<$<CXX_COMPILER_ID:MSVC>:/W4;/permissive->")
target_link_libraries(knight PRIVATE
    "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:${SANITISERS}>")
