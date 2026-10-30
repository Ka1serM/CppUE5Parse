cmake_minimum_required(VERSION 3.28)
project(CppUE5Parse LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(UE5PARSE_BUILD_TOOLS "Build the command line tools" ${PROJECT_IS_TOP_LEVEL})
option(UE5PARSE_BUILD_TESTS "Build the tests" ${PROJECT_IS_TOP_LEVEL})

include(cmake/UnrealHeaderTool.cmake)

add_subdirectory(external)
add_subdirectory(src)

if(UE5PARSE_BUILD_TOOLS)
    add_subdirectory(tools)
endif()
if(UE5PARSE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
