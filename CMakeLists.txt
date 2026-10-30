cmake_minimum_required(VERSION 3.29)
project(CppUE5Parse LANGUAGES C CXX)

include(cmake/CompilerOptions.cmake)
include(cmake/UnrealHeaderTool.cmake)

option(UE5PARSE_BUILD_TOOLS "Build the command line tools" ${PROJECT_IS_TOP_LEVEL})
option(UE5PARSE_BUILD_TESTS "Build the tests" ${PROJECT_IS_TOP_LEVEL})

add_subdirectory(external)
add_subdirectory(src)
if(UE5PARSE_BUILD_TOOLS)
    add_subdirectory(tools)
endif()
if(UE5PARSE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
