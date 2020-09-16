cmake_minimum_required(VERSION 3.0)

set(DEFAULT_BUILD_TYPE "Release")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# library
project(arg_parse_convert)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -std=c++17")

add_library(arg_parse_convert
        "${CMAKE_CURRENT_SOURCE_DIR}/src/argument_map.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/help_string_formatters.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/parameter.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/parameter_map.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/parsers.cc")
target_include_directories(arg_parse_convert PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include")

# unit tests
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    project(arg_parse_convert_test)
    include(CTest)

    set(CMAKE_CXX_STANDARD 17)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -std=c++17")

    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test")
endif()
