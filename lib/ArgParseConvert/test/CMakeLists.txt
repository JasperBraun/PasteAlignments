add_executable(parameter_test
        "${PROJECT_SOURCE_DIR}/test/parameter_test.cc"
        "${PROJECT_SOURCE_DIR}/src/parameter.cc")
target_include_directories(parameter_test PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/lib/catch/include"
        "${PROJECT_SOURCE_DIR}/test")
add_test(NAME parameter_test COMMAND parameter_test)

add_executable(parameter_map_test
        "${PROJECT_SOURCE_DIR}/test/parameter_map_test.cc"
        "${PROJECT_SOURCE_DIR}/src/parameter_map.cc"
        "${PROJECT_SOURCE_DIR}/src/parameter.cc")
target_include_directories(parameter_map_test PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/lib/catch/include"
        "${PROJECT_SOURCE_DIR}/test")
add_test(NAME parameter_map_test COMMAND parameter_map_test)

add_executable(argument_map_test
        "${PROJECT_SOURCE_DIR}/test/argument_map_test.cc"
        "${PROJECT_SOURCE_DIR}/src/argument_map.cc"
        "${PROJECT_SOURCE_DIR}/src/parameter_map.cc"
        "${PROJECT_SOURCE_DIR}/src/parameter.cc")
target_include_directories(argument_map_test PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/lib/catch/include"
        "${PROJECT_SOURCE_DIR}/test")
add_test(NAME argument_map_test COMMAND argument_map_test)

add_executable(parsers_test
        "${PROJECT_SOURCE_DIR}/test/parsers_test.cc"
        "${PROJECT_SOURCE_DIR}/src/parsers.cc"
        "${PROJECT_SOURCE_DIR}/src/argument_map.cc"
        "${PROJECT_SOURCE_DIR}/src/parameter_map.cc"
        "${PROJECT_SOURCE_DIR}/src/parameter.cc")
target_include_directories(parsers_test PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/lib/catch/include"
        "${PROJECT_SOURCE_DIR}/test")
add_test(NAME parsers_test COMMAND parsers_test)

add_executable(help_string_formatters_test
        "${PROJECT_SOURCE_DIR}/test/help_string_formatters_test.cc"
        "${PROJECT_SOURCE_DIR}/src/help_string_formatters.cc"
        "${PROJECT_SOURCE_DIR}/src/parameter_map.cc"
        "${PROJECT_SOURCE_DIR}/src/parameter.cc")
target_include_directories(help_string_formatters_test PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/lib/catch/include"
        "${PROJECT_SOURCE_DIR}/test")
add_test(NAME help_string_formatters_test COMMAND help_string_formatters_test)