add_executable(alignment_test
        "${PROJECT_SOURCE_DIR}/test/alignment_test.cc"
        "${PROJECT_SOURCE_DIR}/src/alignment.cc")
target_include_directories(alignment_test PUBLIC
        "${PROJECT_SOURCE_DIR}/test"
        "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/lib/catch/include")
add_test(NAME alignment_test COMMAND alignment_test)
