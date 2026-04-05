cmake_minimum_required(VERSION 3.16)

project(jorgescript
    VERSION 0.1.0
    DESCRIPTION "The JorgeScript Interpreter"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(RESOURCES_HEADER "${CMAKE_BINARY_DIR}/generated/Resources.hpp")
file(GLOB STDLIB_FILES "${PROJECT_SOURCE_DIR}/stdlib/*.jorge")

add_custom_command(
    OUTPUT "${RESOURCES_HEADER}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/generated"
    COMMAND ${Python3_EXECUTABLE} "${PROJECT_SOURCE_DIR}/generateResources.py" "${RESOURCES_HEADER}" ${STDLIB_FILES}
    DEPENDS ${STDLIB_FILES} "${PROJECT_SOURCE_DIR}/generateResources.py"
    COMMENT "Generating embedded resources"
)

add_executable(jorgescript
    src/main.cpp
    src/frontend/Lexer.cpp
    src/frontend/Parser.cpp
    src/runtime/ValueUtils.cpp
    src/runtime/ClibRuntime.cpp
    src/runtime/modules/IOFunctions.cpp
    src/runtime/modules/StringFunctions.cpp
    src/runtime/modules/TestsFunctions.cpp
    src/ast/ASTExpressions.cpp
    src/runtime/ASTStatements.cpp
    "${RESOURCES_HEADER}"
)

target_include_directories(jorgescript PRIVATE
    ${PROJECT_SOURCE_DIR}/src/frontend
    ${PROJECT_SOURCE_DIR}/src/runtime
    ${PROJECT_SOURCE_DIR}/src/ast
    ${CMAKE_BINARY_DIR}/generated
)

if (MSVC)
    target_compile_options(jorgescript PRIVATE /W4 /permissive-)
else()
    target_compile_options(jorgescript PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (UNIX AND NOT APPLE)
    target_link_libraries(jorgescript PRIVATE dl)
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()
