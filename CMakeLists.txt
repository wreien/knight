cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0092 NEW)

project(knight VERSION 0.1 LANGUAGES CXX)
include(CheckIPOSupported)
include(CheckCXXCompilerFlag)

CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
option(OPTIMISE_NATIVE "Release builds use -march=native, if available"
    ${COMPILER_SUPPORTS_MARCH_NATIVE})

add_executable(knight)
set_target_properties(knight PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    MSVC_RUNTIME_LIBRARY "$<$<CXX_COMPILER_ID:Clang>:MultiThreaded>"
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
    src/ir.cpp
    src/ir.hpp
    src/lexer.cpp
    src/lexer.hpp
    src/main.cpp
    src/parser.cpp
    src/parser.hpp
    src/sourcepos.hpp
    src/value.hpp
    src/value.cpp
)

if(WIN32)
    target_sources(knight PRIVATE src/shell_win32.cpp)
else()
    target_sources(knight PRIVATE src/shell_posix.cpp)
endif()

check_ipo_supported(RESULT IS_IPO_SUPPORTED)
if(IS_IPO_SUPPORTED)
    set_target_properties(knight PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE YES
        INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO YES)
endif()

set(SANITISERS "$<$<CONFIG:Debug>:-fsanitize=undefined,address;-fno-omit-frame-pointer>")

target_compile_options(knight PRIVATE
    "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-pedantic;-Wsign-conversion;${SANITISERS}>"
    "$<$<CXX_COMPILER_ID:MSVC>:/W4;/permissive->")
target_link_libraries(knight PRIVATE
    "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:${SANITISERS}>")

if(OPTIMISE_NATIVE AND COMPILER_SUPPORTS_MARCH_NATIVE)
    target_compile_options(knight PRIVATE "$<$<CONFIG:Release>:-march=native>")
endif()
