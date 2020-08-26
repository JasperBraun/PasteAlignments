add_executable(helpers_test
        "${PROJECT_SOURCE_DIR}/test/helpers_test.cc"
        "${PROJECT_SOURCE_DIR}/src/helpers.cc")
target_include_directories(helpers_test PUBLIC
        "${PROJECT_SOURCE_DIR}/test"
        "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/lib/catch/include")
add_test(NAME helpers_test COMMAND helpers_test)

add_executable(alignment_test
        "${PROJECT_SOURCE_DIR}/test/alignment_test.cc"
        "${PROJECT_SOURCE_DIR}/src/alignment.cc"
        "${PROJECT_SOURCE_DIR}/src/helpers.cc")
target_include_directories(alignment_test PUBLIC
        "${PROJECT_SOURCE_DIR}/test"
        "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/lib/catch/include")
add_test(NAME alignment_test COMMAND alignment_test)

add_executable(scoring_system_test
        "${PROJECT_SOURCE_DIR}/test/scoring_system_test.cc"
        "${PROJECT_SOURCE_DIR}/src/scoring_system.cc"
        "${PROJECT_SOURCE_DIR}/src/alignment.cc"
        "${PROJECT_SOURCE_DIR}/src/helpers.cc")
target_include_directories(scoring_system_test PUBLIC
        "${PROJECT_SOURCE_DIR}/test"
        "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/lib/catch/include")
add_test(NAME scoring_system_test COMMAND scoring_system_test)
