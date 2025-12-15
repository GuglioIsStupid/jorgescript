cmake_minimum_required(VERSION 3.16)

project(jorgescript
    VERSION 0.1.0
    DESCRIPTION "The JorgeScript Interpreter"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(jorgescript
    src/main.cpp
    src/Lexer.cpp
    src/Parser.cpp
    src/AST.cpp
)

target_include_directories(jorgescript PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

if (MSVC)
    target_compile_options(jorgescript PRIVATE /W4 /permissive-)
else()
    target_compile_options(jorgescript PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()
