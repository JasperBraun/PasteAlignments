cmake_minimum_required(VERSION 3.0)

# application
project(paste_alignments)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -std=c++17")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/lib")

set(DEFAULT_BUILD_TYPE "Debug")

add_executable(paste_alignments
        "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/alignment.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/alignment_batch.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/alignment_reader.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/helpers.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/paste_output.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/scoring_system.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/stats_collector.cc")
target_include_directories(paste_alignments PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/ArgParseConvert/include")
target_link_libraries(paste_alignments arg_parse_convert)


if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    project(paste_alignments_test)
    include(CTest)

    set(CMAKE_CXX_STANDARD 17)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -std=c++17")

    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test")
endif()